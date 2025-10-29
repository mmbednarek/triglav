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
   // ctx.declare_buffer("render_viewport"_name);
   // ctx.add_texture_usage("render_viewport.out"_name, graphics_api::TextureUsage::Sampled);

   const auto box_mesh = geometry::create_box({20.0f, 20.0f, 20.0f});
   box_mesh.triangulate();
   const auto vert_data = box_mesh.to_vertex_data();

   std::vector<Vector3> vertices(vert_data.vertices.size());
   std::ranges::transform(vert_data.vertices, vertices.begin(), [&](const geometry::Vertex& v) { return v.location; });
   ctx.init_buffer_raw("render_viewport.box_vertices"_name, vertices.data(), vertices.size() * sizeof(Vector3));
   ctx.init_buffer_raw("render_viewport.box_indices"_name, vert_data.indices.data(), vert_data.indices.size() * sizeof(u32));

   Matrix4x4 iden{20};
   ctx.init_buffer("render_viewport.ubo"_name, iden);

   m_levelEditor.m_renderingJob.build_job(ctx);

   ctx.blit_texture("shading.color"_name, "render_viewport.out"_name);

   ctx.begin_render_pass("editor_tools"_name, "render_viewport.out"_name);

   ctx.bind_vertex_shader("editor/object_selection.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);
   ctx.bind_uniform_buffer(1, "render_viewport.ubo"_name);

   triglav::render_core::VertexLayout layout(sizeof(Vector3));
   layout.add("position"_name, GAPI_FORMAT(RGB, Float32), 0);
   ctx.bind_vertex_layout(layout);

   ctx.bind_vertex_buffer("render_viewport.box_vertices"_name);
   ctx.bind_index_buffer("render_viewport.box_indices"_name);

   ctx.bind_fragment_shader("editor/object_selection.fshader"_rc);

   ctx.set_vertex_topology(graphics_api::VertexTopology::LineStrip);

   ctx.draw_indexed_primitives(static_cast<u32>(vert_data.indices.size()), 0, 0);

   ctx.end_render_pass();

   ctx.export_texture("render_viewport.out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void RenderViewport::update(render_core::JobGraph& graph, const u32 frameIndex, const float deltaTime)
{
   m_levelEditor.m_updateViewParamsJob.prepare_frame(graph, frameIndex, deltaTime);
}

[[nodiscard]] Vector4 RenderViewport::dimensions() const
{
   return m_dimensions;
}

}// namespace triglav::editor
