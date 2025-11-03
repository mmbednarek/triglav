#include "RenderViewport.hpp"

#include "ToolMeshes.hpp"
#include "RootWindow.hpp"

#include "triglav/Ranges.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace render_core::literals;

namespace {

render_core::VertexLayout g_singleVertexLayout =
   render_core::VertexLayout(sizeof(Vector3)).add("position"_name, GAPI_FORMAT(RGB, Float32), 0);

void render_tool(render_core::BuildContext& ctx, const Name vertices, const Name indices, const u32 index, const u32 index_count, MemorySize color_stride, MemorySize matrix_stride,
                 const graphics_api::VertexTopology topology, bool enable_blending)
{
   ctx.bind_vertex_shader("editor/object_selection.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);
   ctx.bind_uniform_buffer(1, "render_viewport.colors"_name, static_cast<u32>(index * color_stride), sizeof(Vector4));
   ctx.bind_uniform_buffer(2, "render_viewport.matrices"_name, static_cast<u32>(index * matrix_stride), sizeof(Matrix4x4));

   ctx.bind_vertex_layout(g_singleVertexLayout);

   ctx.bind_vertex_buffer(vertices);
   ctx.bind_index_buffer(indices);

   ctx.bind_fragment_shader("editor/object_selection.fshader"_rc);
   ctx.bind_samplable_texture(3, "gbuffer.depth"_name);

   ctx.set_vertex_topology(topology);
   ctx.set_is_blending_enabled(enable_blending);

   ctx.draw_indexed_primitives(index_count, 0, 0);
}

}// namespace

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

   const auto [vertices_box, indices_box] = create_box_mesh();
   ctx.init_buffer_raw("render_viewport.box_vertices"_name, vertices_box.data(), vertices_box.size() * sizeof(Vector3));
   ctx.init_buffer_raw("render_viewport.box_indices"_name, indices_box.data(), indices_box.size() * sizeof(u32));

   const auto [vertices_arrow, indices_arrow] = create_arrow_mesh<12>({0, 0, 0}, 0.08f, 3.0f, 0.25f, 1.3f);
   ctx.init_buffer_raw("render_viewport.arrow_vertices"_name, vertices_arrow.data(), vertices_arrow.size() * sizeof(Vector3));
   ctx.init_buffer_raw("render_viewport.arrow_indices"_name, indices_arrow.data(), indices_arrow.size() * sizeof(u32));

   const auto& limits = m_levelEditor.m_state.rootWindow->device().limits();

   const auto color_align = align_size(sizeof(Vector4), limits.min_uniform_buffer_alignment);
   ctx.declare_staging_buffer("render_viewport.colors.staging"_name, color_align * m_colors.size());
   ctx.declare_buffer("render_viewport.colors"_name, color_align * m_colors.size());

   const auto matrix_align = align_size(sizeof(Matrix4x4), limits.min_uniform_buffer_alignment);
   ctx.declare_staging_buffer("render_viewport.matrices.staging"_name, matrix_align * m_matrices.size());
   ctx.declare_buffer("render_viewport.matrices"_name, matrix_align * m_matrices.size());

   m_levelEditor.m_renderingJob.build_job(ctx);

   ctx.copy_buffer("render_viewport.colors.staging"_name, "render_viewport.colors"_name);
   ctx.copy_buffer("render_viewport.matrices.staging"_name, "render_viewport.matrices"_name);

   ctx.blit_texture("shading.color"_name, "render_viewport.out"_name);

   ctx.begin_render_pass("editor_tools"_name, "render_viewport.out"_name);

   render_tool(ctx, "render_viewport.arrow_vertices"_name, "render_viewport.arrow_indices"_name, 1, static_cast<u32>(indices_arrow.size()),
               color_align, matrix_align, graphics_api::VertexTopology::TriangleList, false);
   render_tool(ctx, "render_viewport.arrow_vertices"_name, "render_viewport.arrow_indices"_name, 2, static_cast<u32>(indices_arrow.size()),
               color_align, matrix_align, graphics_api::VertexTopology::TriangleList, false);
   render_tool(ctx, "render_viewport.arrow_vertices"_name, "render_viewport.arrow_indices"_name, 3, static_cast<u32>(indices_arrow.size()),
               color_align, matrix_align, graphics_api::VertexTopology::TriangleList, false);

   render_tool(ctx, "render_viewport.box_vertices"_name, "render_viewport.box_indices"_name, 0, static_cast<u32>(indices_box.size()),
               color_align, matrix_align, graphics_api::VertexTopology::LineList, true);

   ctx.end_render_pass();

   ctx.export_texture("render_viewport.out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void RenderViewport::update(render_core::JobGraph& graph, const u32 frameIndex, const float deltaTime)
{
   m_levelEditor.m_updateViewParamsJob.prepare_frame(graph, frameIndex, deltaTime);

   if (m_updates < render_core::FRAMES_IN_FLIGHT_COUNT) {
	   const auto& limits = m_levelEditor.m_state.rootWindow->device().limits();
	   const auto color_align = align_size(sizeof(Vector4), limits.min_uniform_buffer_alignment);
	   const auto matrix_align = align_size(sizeof(Matrix4x4), limits.min_uniform_buffer_alignment);

      const auto matrices_mapping = GAPI_CHECK(graph.resources().buffer("render_viewport.matrices.staging"_name, frameIndex).map_memory());
      for (const auto& [index, matrix] : Enumerate(m_matrices)) {
         std::memcpy(static_cast<char*>(*matrices_mapping) + matrix_align * index, &m_matrices[index], sizeof(Matrix4x4));
      }

      const auto colors_mapping = GAPI_CHECK(graph.resources().buffer("render_viewport.colors.staging"_name, frameIndex).map_memory());
      for (const auto& [index, color] : Enumerate(m_colors)) {
         std::memcpy(static_cast<char*>(*colors_mapping) + color_align * index, &m_colors[index], sizeof(Color));
      }

      ++m_updates;
   }
}

[[nodiscard]] Vector4 RenderViewport::dimensions() const
{
   return m_dimensions;
}

void RenderViewport::set_selection_matrix(const u32 index, const Matrix4x4& mat)
{
   m_matrices[index] = mat;
   m_updates = 0;
}


}// namespace triglav::editor
