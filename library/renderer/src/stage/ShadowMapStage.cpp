#include "stage/ShadowMapStage.hpp"

#include "BindlessScene.hpp"
#include "Scene.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

namespace gapi = graphics_api;

static render_core::VertexLayout g_vertexLayout =
   render_core::VertexLayout(sizeof(geometry::Vertex)).add("position"_name, GAPI_FORMAT(RGBA, Float32), 0);

constexpr graphics_api::SamplerProperties g_shadowMapProps{
   .minFilter = graphics_api::FilterType::Linear,
   .magFilter = graphics_api::FilterType::Linear,
   .addressU = graphics_api::TextureAddressMode::Clamp,
   .addressV = graphics_api::TextureAddressMode::Clamp,
   .addressW = graphics_api::TextureAddressMode::Clamp,
   .enableAnisotropy = false,
   .maxLod = 1.0f,
};

constexpr Vector2i g_shadowMapSize{4096, 4096};

ShadowMapStage::ShadowMapStage(Scene& scene, BindlessScene& bindlessScene, UpdateViewParamsJob& updateViewParamsJob) :
    m_scene(scene),
    m_bindlessScene(bindlessScene),
    TG_CONNECT(updateViewParamsJob, OnResourceDefinition, on_resource_definition),
    TG_CONNECT(updateViewParamsJob, OnViewPropertiesChanged, on_view_properties_changed),
    TG_CONNECT(updateViewParamsJob, OnViewPropertiesNotChanged, on_view_properties_not_changed),
    TG_CONNECT(updateViewParamsJob, OnFinalize, on_finalize),
    TG_CONNECT(updateViewParamsJob, OnPrepareFrame, on_prepare_frame)
{
}

void ShadowMapStage::build_stage(render_core::BuildContext& ctx, const Config& /*config*/) const
{
   ctx.declare_sized_depth_target("shadow_map.cascade0"_name, g_shadowMapSize, GAPI_FORMAT(D, UNorm16));
   ctx.declare_sized_depth_target("shadow_map.cascade1"_name, g_shadowMapSize, GAPI_FORMAT(D, UNorm16));
   ctx.declare_sized_depth_target("shadow_map.cascade2"_name, g_shadowMapSize, GAPI_FORMAT(D, UNorm16));

   ctx.set_sampler_properties("shadow_map.cascade0"_name, g_shadowMapProps);
   ctx.set_sampler_properties("shadow_map.cascade1"_name, g_shadowMapProps);
   ctx.set_sampler_properties("shadow_map.cascade2"_name, g_shadowMapProps);

   this->render_cascade(ctx, "shadow_map.pass.cascade0"_name, "shadow_map.cascade0"_name, "shadow_map.view_properties.cascade0"_external);
   this->render_cascade(ctx, "shadow_map.pass.cascade1"_name, "shadow_map.cascade1"_name, "shadow_map.view_properties.cascade1"_external);
   this->render_cascade(ctx, "shadow_map.pass.cascade2"_name, "shadow_map.cascade2"_name, "shadow_map.view_properties.cascade2"_external);
}

void ShadowMapStage::render_cascade(render_core::BuildContext& ctx, const Name passName, const Name targetName,
                                    const render_core::BufferRef viewProps) const
{
   render_core::RenderPassScope rtScope(ctx, passName, targetName);

   ctx.bind_vertex_shader("bindless_geometry/shadow_map.vshader"_rc);

   ctx.bind_vertex_layout(g_vertexLayout);

   ctx.bind_uniform_buffer(0, viewProps);
   ctx.bind_storage_buffer(1, &m_bindlessScene.scene_object_buffer());

   ctx.bind_fragment_shader("bindless_geometry/shadow_map.fshader"_rc);

   ctx.bind_vertex_buffer(&m_bindlessScene.combined_vertex_buffer());
   ctx.bind_index_buffer(&m_bindlessScene.combined_index_buffer());

   ctx.draw_indexed_indirect_with_count(&m_bindlessScene.scene_object_buffer(), &m_bindlessScene.count_buffer(), 128,
                                        sizeof(BindlessSceneObject));
}

void ShadowMapStage::on_resource_definition(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("shadow_map.matrices"_name, 3 * sizeof(Matrix4x4));
   ctx.declare_staging_buffer("shadow_map.matrices.staging"_name, 3 * sizeof(Matrix4x4));

   ctx.declare_buffer("shadow_map.view_properties.cascade0"_name, sizeof(Matrix4x4));
   ctx.declare_staging_buffer("shadow_map.view_properties.cascade0.staging"_name, sizeof(Matrix4x4));
   ctx.declare_buffer("shadow_map.view_properties.cascade1"_name, sizeof(Matrix4x4));
   ctx.declare_staging_buffer("shadow_map.view_properties.cascade1.staging"_name, sizeof(Matrix4x4));
   ctx.declare_buffer("shadow_map.view_properties.cascade2"_name, sizeof(Matrix4x4));
   ctx.declare_staging_buffer("shadow_map.view_properties.cascade2.staging"_name, sizeof(Matrix4x4));
}

void ShadowMapStage::on_view_properties_changed(render_core::BuildContext& ctx) const
{
   ctx.copy_buffer("shadow_map.matrices.staging"_name, "shadow_map.matrices"_name);
   ctx.copy_buffer("shadow_map.view_properties.cascade0.staging"_name, "shadow_map.view_properties.cascade0"_name);
   ctx.copy_buffer("shadow_map.view_properties.cascade1.staging"_name, "shadow_map.view_properties.cascade1"_name);
   ctx.copy_buffer("shadow_map.view_properties.cascade2.staging"_name, "shadow_map.view_properties.cascade2"_name);
}

void ShadowMapStage::on_view_properties_not_changed(render_core::BuildContext& ctx) const
{
   ctx.copy_buffer("shadow_map.matrices"_last_frame, "shadow_map.matrices"_name);
   ctx.copy_buffer("shadow_map.view_properties.cascade0"_last_frame, "shadow_map.view_properties.cascade0"_name);
   ctx.copy_buffer("shadow_map.view_properties.cascade1"_last_frame, "shadow_map.view_properties.cascade1"_name);
   ctx.copy_buffer("shadow_map.view_properties.cascade2"_last_frame, "shadow_map.view_properties.cascade2"_name);
}

void ShadowMapStage::on_finalize(render_core::BuildContext& ctx) const
{
   ctx.export_buffer("shadow_map.matrices"_name, gapi::PipelineStage::VertexShader, gapi::BufferAccess::UniformRead,
                     gapi::BufferUsage::UniformBuffer);
   ctx.export_buffer("shadow_map.view_properties.cascade0"_name, gapi::PipelineStage::VertexShader, gapi::BufferAccess::UniformRead,
                     gapi::BufferUsage::UniformBuffer);
   ctx.export_buffer("shadow_map.view_properties.cascade1"_name, gapi::PipelineStage::VertexShader, gapi::BufferAccess::UniformRead,
                     gapi::BufferUsage::UniformBuffer);
   ctx.export_buffer("shadow_map.view_properties.cascade2"_name, gapi::PipelineStage::VertexShader, gapi::BufferAccess::UniformRead,
                     gapi::BufferUsage::UniformBuffer);
}

void ShadowMapStage::on_prepare_frame(render_core::JobGraph& graph, const u32 frameIndex) const
{
   const auto inverseViewMat = glm::inverse(m_scene.camera().view_matrix());

   const auto mappedMem = GAPI_CHECK(graph.resources().buffer("shadow_map.matrices.staging"_name, frameIndex).map_memory());
   auto& matrices = mappedMem.cast<std::array<Matrix4x4, 3>>();
   matrices[0] = m_scene.shadow_map_camera(0).view_projection_matrix() * inverseViewMat;
   matrices[1] = m_scene.shadow_map_camera(1).view_projection_matrix() * inverseViewMat;
   matrices[2] = m_scene.shadow_map_camera(2).view_projection_matrix() * inverseViewMat;

   const auto mappedMemViewProps0 =
      GAPI_CHECK(graph.resources().buffer("shadow_map.view_properties.cascade0.staging"_name, frameIndex).map_memory());
   mappedMemViewProps0.cast<Matrix4x4>() = m_scene.shadow_map_camera(0).view_projection_matrix();
   const auto mappedMemViewProps1 =
      GAPI_CHECK(graph.resources().buffer("shadow_map.view_properties.cascade1.staging"_name, frameIndex).map_memory());
   mappedMemViewProps1.cast<Matrix4x4>() = m_scene.shadow_map_camera(1).view_projection_matrix();
   const auto mappedMemViewProps2 =
      GAPI_CHECK(graph.resources().buffer("shadow_map.view_properties.cascade2.staging"_name, frameIndex).map_memory());
   mappedMemViewProps2.cast<Matrix4x4>() = m_scene.shadow_map_camera(2).view_projection_matrix();
}

}// namespace triglav::renderer::stage
