#include "RenderViewport.hpp"

#include "triglav/geometry/DebugMesh.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace render_core::literals;

namespace {

template<u32 CRingCount>
auto create_arrow_mesh(Vector3 origin, float shaft_radius, float shaft_height, float cone_radius, float cone_height)
{
   static constexpr auto VERTEX_COUNT = 3 + 3 * CRingCount;
   static constexpr auto INDEX_COUNT = 5 * 3 * CRingCount;
   static constexpr auto ORIGIN_IDX = 0;
   static constexpr auto MID_IDX = 1;
   static constexpr auto TOP_IDX = 2;
   static constexpr auto SHAFT_BOTTOM_IDX = 3;
   static constexpr auto SHAFT_TOP_IDX = 3 + CRingCount;
   static constexpr auto CONE_IDX = 3 + 2*CRingCount;

   std::array<Vector3, VERTEX_COUNT> vertices;
   std::array<u32, INDEX_COUNT> indices;
   vertices[ORIGIN_IDX] = origin;
   vertices[MID_IDX] = origin + Vector3(0, 0, shaft_height);
   vertices[TOP_IDX] = origin + Vector3(0, 0, shaft_height + cone_height);

   for (int i = 0; i < CRingCount; ++i) {
      const auto next = (i + 1) % CRingCount;
      const float angle = i * (2 * g_pi / CRingCount);
      vertices[SHAFT_BOTTOM_IDX + i] = origin + Vector3(shaft_radius * std::sin(angle), shaft_radius * std::cos(angle), 0);
      vertices[SHAFT_TOP_IDX + i] = origin + Vector3(shaft_radius * std::sin(angle), shaft_radius * std::cos(angle), shaft_height);
      vertices[CONE_IDX + i] = origin + Vector3(cone_radius * std::sin(angle), cone_radius * std::cos(angle), shaft_height);

      // Shaft back
      indices[3 * i] = SHAFT_BOTTOM_IDX + i;
      indices[3 * i + 1] = ORIGIN_IDX;
      indices[3 * i + 2] = SHAFT_BOTTOM_IDX + next;

      indices[3 * CRingCount + 6 * i] = SHAFT_BOTTOM_IDX + i;
      indices[3 * CRingCount + 6 * i + 1] = SHAFT_BOTTOM_IDX + next;
      indices[3 * CRingCount + 6 * i + 2] = SHAFT_TOP_IDX + i;
      indices[3 * CRingCount + 6 * i + 3] = SHAFT_BOTTOM_IDX + next;
      indices[3 * CRingCount + 6 * i + 4] = SHAFT_TOP_IDX + next;
      indices[3 * CRingCount + 6 * i + 5] = SHAFT_TOP_IDX + i;

      indices[9 * CRingCount + 3 * i] = CONE_IDX + i;
      indices[9 * CRingCount + 3 * i + 1] = MID_IDX;
      indices[9 * CRingCount + 3 * i + 2] = CONE_IDX + next;

      indices[12 * CRingCount + 3 * i] = CONE_IDX + i;
      indices[12 * CRingCount + 3 * i + 1] = CONE_IDX + next;
      indices[12 * CRingCount + 3 * i + 2] = TOP_IDX;
   }

   return std::make_pair(vertices, indices);
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

   const auto [vertices, indices] = create_arrow_mesh<24>({0, 0, 0}, 0.2f, 2.0f, 0.6f, 1.5f);

   // static constexpr std::array vertices{
   //    Vector3{0, 0, 1}, Vector3{0, 1, 1}, Vector3{0, 0, 0}, Vector3{0, 1, 0},
   //    Vector3{1, 0, 1}, Vector3{1, 1, 1}, Vector3{1, 0, 0}, Vector3{1, 1, 0},
   // };
   // static constexpr std::array<u32, 24> indices{0, 1, 1, 3, 3, 2, 2, 0, 4, 5, 5, 7, 7, 6, 6, 4, 0, 4, 1, 5, 2, 6, 3, 7};

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

   // ctx.set_vertex_topology(graphics_api::VertexTopology::LineList);
   ctx.set_is_blending_enabled(true);

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
