#include "stage/ShadowMapStage.hpp"

#include "BindlessScene.hpp"
#include "Scene.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

namespace gapi = graphics_api;

static render_core::VertexLayout g_vertex_layout =
   render_core::VertexLayout(sizeof(geometry::Vertex)).add("position"_name, GAPI_FORMAT(RGBA, Float32), 0);

constexpr graphics_api::SamplerProperties g_shadow_map_props{
   .min_filter = graphics_api::FilterType::Linear,
   .mag_filter = graphics_api::FilterType::Linear,
   .address_u = graphics_api::TextureAddressMode::Clamp,
   .address_v = graphics_api::TextureAddressMode::Clamp,
   .address_w = graphics_api::TextureAddressMode::Clamp,
   .enable_anisotropy = false,
   .max_lod = 1.0f,
};

constexpr Vector2i g_shadow_map_size{4096, 4096};

ShadowMapStage::ShadowMapStage(Scene& scene, BindlessScene& bindless_scene, UpdateViewParamsJob& update_view_params_job) :
    m_scene(scene),
    m_bindless_scene(bindless_scene),
    TG_CONNECT(update_view_params_job, OnResourceDefinition, on_resource_definition),
    TG_CONNECT(update_view_params_job, OnViewPropertiesChanged, on_view_properties_changed),
    TG_CONNECT(update_view_params_job, OnViewPropertiesNotChanged, on_view_properties_not_changed),
    TG_CONNECT(update_view_params_job, OnFinalize, on_finalize),
    TG_CONNECT(update_view_params_job, OnPrepareFrame, on_prepare_frame)
{
}

void ShadowMapStage::build_stage(render_core::BuildContext& ctx, const Config& /*config*/) const
{
   ctx.declare_sized_depth_target("shadow_map.cascade0"_name, g_shadow_map_size, GAPI_FORMAT(D, UNorm16));
   ctx.declare_sized_depth_target("shadow_map.cascade1"_name, g_shadow_map_size, GAPI_FORMAT(D, UNorm16));
   ctx.declare_sized_depth_target("shadow_map.cascade2"_name, g_shadow_map_size, GAPI_FORMAT(D, UNorm16));

   ctx.set_sampler_properties("shadow_map.cascade0"_name, g_shadow_map_props);
   ctx.set_sampler_properties("shadow_map.cascade1"_name, g_shadow_map_props);
   ctx.set_sampler_properties("shadow_map.cascade2"_name, g_shadow_map_props);

   this->render_cascade(ctx, "shadow_map.pass.cascade0"_name, "shadow_map.cascade0"_name, "shadow_map.view_properties.cascade0"_external);
   this->render_cascade(ctx, "shadow_map.pass.cascade1"_name, "shadow_map.cascade1"_name, "shadow_map.view_properties.cascade1"_external);
   this->render_cascade(ctx, "shadow_map.pass.cascade2"_name, "shadow_map.cascade2"_name, "shadow_map.view_properties.cascade2"_external);
}

void ShadowMapStage::render_cascade(render_core::BuildContext& ctx, const Name pass_name, const Name target_name,
                                    const render_core::BufferRef view_props) const
{
   render_core::RenderPassScope rt_scope(ctx, pass_name, target_name);

   ctx.bind_vertex_shader("bindless_geometry/shadow_map.vshader"_rc);

   ctx.bind_vertex_layout(g_vertex_layout);

   ctx.bind_uniform_buffer(0, view_props);
   ctx.bind_storage_buffer(1, &m_bindless_scene.scene_object_buffer());

   ctx.bind_fragment_shader("bindless_geometry/shadow_map.fshader"_rc);

   ctx.bind_vertex_buffer(&m_bindless_scene.combined_vertex_buffer());
   ctx.bind_index_buffer(&m_bindless_scene.combined_index_buffer());

   ctx.draw_indexed_indirect_with_count(&m_bindless_scene.scene_object_buffer(), &m_bindless_scene.count_buffer(), 128,
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

void ShadowMapStage::on_prepare_frame(render_core::JobGraph& graph, const u32 frame_index) const
{
   const auto inverse_view_mat = glm::inverse(m_scene.camera().view_matrix());

   const auto mapped_mem = GAPI_CHECK(graph.resources().buffer("shadow_map.matrices.staging"_name, frame_index).map_memory());
   auto& matrices = mapped_mem.cast<std::array<Matrix4x4, 3>>();
   matrices[0] = m_scene.shadow_map_camera(0).view_projection_matrix() * inverse_view_mat;
   matrices[1] = m_scene.shadow_map_camera(1).view_projection_matrix() * inverse_view_mat;
   matrices[2] = m_scene.shadow_map_camera(2).view_projection_matrix() * inverse_view_mat;

   const auto mapped_mem_view_props0 =
      GAPI_CHECK(graph.resources().buffer("shadow_map.view_properties.cascade0.staging"_name, frame_index).map_memory());
   mapped_mem_view_props0.cast<Matrix4x4>() = m_scene.shadow_map_camera(0).view_projection_matrix();
   const auto mapped_mem_view_props1 =
      GAPI_CHECK(graph.resources().buffer("shadow_map.view_properties.cascade1.staging"_name, frame_index).map_memory());
   mapped_mem_view_props1.cast<Matrix4x4>() = m_scene.shadow_map_camera(1).view_projection_matrix();
   const auto mapped_mem_view_props2 =
      GAPI_CHECK(graph.resources().buffer("shadow_map.view_properties.cascade2.staging"_name, frame_index).map_memory());
   mapped_mem_view_props2.cast<Matrix4x4>() = m_scene.shadow_map_camera(2).view_projection_matrix();
}

}// namespace triglav::renderer::stage
