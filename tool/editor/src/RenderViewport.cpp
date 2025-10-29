#include "RenderViewport.hpp"

#include "triglav/geometry/DebugMesh.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace render_core::literals;

RenderViewport::RenderViewport(LevelEditor& levelEditor, const Vector4 dimensions) :
    m_levelEditor(levelEditor),
    m_dimensions(dimensions)
{
}

void RenderViewport::build_update_job(render_core::BuildContext& ctx)
{
   m_levelEditor.m_updateViewParamsJob.build_job(ctx);
}

struct EditorOverlayUniformBuffer
{
   Matrix4x4 select_object_transform;
};

void RenderViewport::build_render_job(render_core::BuildContext& ctx)
{
   ctx.declare_render_target("render_viewport.out"_name, GAPI_FORMAT(BGRA, sRGB));

   static constexpr std::array vertices{
      Vector3{0, 0, 1}, Vector3{0, 1, 1}, Vector3{0, 0, 0}, Vector3{0, 1, 0},
      Vector3{1, 0, 1}, Vector3{1, 1, 1}, Vector3{1, 0, 0}, Vector3{1, 1, 0},
   };
   static constexpr std::array<u32, 24> indices{0, 1, 1, 3, 3, 2, 2, 0, 4, 5, 5, 7, 7, 6, 6, 4, 0, 4, 1, 5, 2, 6, 3, 7};

   ctx.init_buffer_raw("render_viewport.box_vertices"_name, vertices.data(), vertices.size() * sizeof(Vector3));
   ctx.init_buffer_raw("render_viewport.box_indices"_name, indices.data(), indices.size() * sizeof(u32));

   ctx.declare_staging_buffer("render_viewport.ubo.staging"_name, sizeof(Matrix4x4));
   ctx.declare_buffer("render_viewport.ubo"_name, sizeof(Matrix4x4));

   m_levelEditor.m_renderingJob.build_job(ctx);

   ctx.copy_buffer("render_viewport.ubo.staging"_name, "render_viewport.ubo"_name);

   ctx.blit_texture("shading.color"_name, "render_viewport.out"_name);

   ctx.begin_render_pass("editor_tools"_name, "render_viewport.out"_name);

   ctx.bind_vertex_shader("editor/object_selection.vshader"_rc);

   ctx.set_bind_stages(graphics_api::PipelineStage::VertexShader | graphics_api::PipelineStage::FragmentShader);
   ctx.bind_uniform_buffer(0, "core.view_properties"_external);
   ctx.set_bind_stages(graphics_api::PipelineStage::VertexShader);
   ctx.bind_uniform_buffer(1, "render_viewport.ubo"_name);

   triglav::render_core::VertexLayout layout(sizeof(Vector3));
   layout.add("position"_name, GAPI_FORMAT(RGB, Float32), 0);
   ctx.bind_vertex_layout(layout);

   ctx.bind_vertex_buffer("render_viewport.box_vertices"_name);
   ctx.bind_index_buffer("render_viewport.box_indices"_name);

   ctx.bind_fragment_shader("editor/object_selection.fshader"_rc);
   ctx.bind_samplable_texture(2, "shading.color"_name);
   ctx.bind_samplable_texture(3, "gbuffer.depth"_name);

   ctx.set_vertex_topology(graphics_api::VertexTopology::LineList);

   ctx.draw_indexed_primitives(static_cast<u32>(indices.size()), 0, 0);

   ctx.end_render_pass();

   ctx.export_texture("render_viewport.out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void RenderViewport::update(render_core::JobGraph& graph, const u32 frameIndex, const float deltaTime)
{
   m_levelEditor.m_updateViewParamsJob.prepare_frame(graph, frameIndex, deltaTime);

   if (m_selectionMatrixUpdates < render_core::FRAMES_IN_FLIGHT_COUNT) {
      auto& buffer = graph.resources().buffer("render_viewport.ubo.staging"_name, frameIndex);
      const auto mapping = GAPI_CHECK(buffer.map_memory());
      mapping.cast<Matrix4x4>() = m_selectionMatrix;
      ++m_selectionMatrixUpdates;
   }
}

[[nodiscard]] Vector4 RenderViewport::dimensions() const
{
   return m_dimensions;
}

void RenderViewport::set_selection_matrix(const Matrix4x4& mat)
{
   m_selectionMatrix = mat;
   m_selectionMatrixUpdates = 0;
}

}// namespace triglav::editor
