#include "ui/RectangleRenderer.hpp"

#include "triglav/UpdateList.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer::ui {

using namespace name_literals;
using namespace render_core::literals;

using ui_core::RectId;

// Staging insertion buffer
// GPU insertion buffer
constexpr auto g_insertion_buffer_size = 64;

// Staging removal buffer
// GPU removal buffer
constexpr auto g_removal_buffer_size = 32;

// Draw call Buffer
constexpr auto g_draw_call_buffer_size = 512;

constexpr auto g_cs_group_size = 256;

struct DrawCall
{
   u32 vertex_count;
   u32 instance_count;
   u32 first_vertex;
   u32 first_instance;

   RectPrimitive primitive;
};

static_assert(sizeof(DrawCall) % 16 == 0);

namespace {

RectPrimitive to_primitive(const ui_core::Rectangle& rect)
{
   return {
      .dimensions = rect.rect,
      .border_radius = rect.border_radius,
      .border_color = rect.border_color,
      .background_color = rect.color,
      .cropping_mask = rect.crop,
      .border_width = rect.border_width,
   };
}

}// namespace

RectangleRenderer::RectangleRenderer(ui_core::Viewport& viewport) :
    TG_CONNECT(viewport, OnAddedRectangle, on_added_rectangle),
    TG_CONNECT(viewport, OnUpdatedRectangle, on_updated_rectangle),
    TG_CONNECT(viewport, OnRemovedRectangle, on_removed_rectangle)
{
}

void RectangleRenderer::on_added_rectangle(const RectId rect_id, const ui_core::Rectangle& rect)
{
   this->on_updated_rectangle(rect_id, rect);
}

void RectangleRenderer::on_updated_rectangle(const RectId rect_id, const ui_core::Rectangle& rect)
{
   std::unique_lock lk{m_rect_update_mtx};
   for (auto& updates : m_frame_updates) {
      updates.add_or_update(rect_id, to_primitive(rect));
   }
}

void RectangleRenderer::on_removed_rectangle(const RectId rect_id)
{
   std::unique_lock lk{m_rect_update_mtx};
   for (auto& updates : m_frame_updates) {
      updates.remove(rect_id);
   }
}

void RectangleRenderer::set_object(const u32 index, const RectPrimitive& prim)
{
   assert(index <= g_draw_call_buffer_size);
   m_staging_insertions[m_staging_insertions_top].dst_index = index;
   m_staging_insertions[m_staging_insertions_top].primitive = prim;
   m_staging_insertions_top++;
}

void RectangleRenderer::move_object(const u32 src, const u32 dst)
{
   assert(src <= g_draw_call_buffer_size);
   assert(dst <= g_draw_call_buffer_size);
   m_staging_removals[m_staging_removals_top].src_id = src;
   m_staging_removals[m_staging_removals_top].dst_id = dst;
   m_staging_removals_top++;
}

void RectangleRenderer::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   // Insertions
   const auto insertions =
      GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.insertion.staging"_name, frame_index).map_memory());
   m_staging_insertions = &insertions.cast<RectWriteData>();
   m_staging_insertions_top = 0;

   // Removals
   const auto removals = GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.removal.staging"_name, frame_index).map_memory());
   m_staging_removals = &removals.cast<RectCopyInfo>();
   m_staging_removals_top = 0;

   m_frame_updates[frame_index].write_to_buffers(*this);

   // Write calls
   const auto insertion_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.insertion.indirect_buffer"_name, frame_index).map_memory());
   insertion_dims.cast<Vector3u>() = {divide_rounded_up(m_staging_insertions_top, g_cs_group_size), 1, 1};
   const auto insertion_count =
      GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.insertion.count"_name, frame_index).map_memory());
   insertion_count.cast<u32>() = m_staging_insertions_top;

   const auto removal_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.removal.indirect_buffer"_name, frame_index).map_memory());
   removal_dims.cast<Vector3u>() = {divide_rounded_up(m_staging_removals_top, g_cs_group_size), 1, 1};
   const auto removal_count = GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.removal.count"_name, frame_index).map_memory());
   removal_count.cast<u32>() = m_staging_removals_top;

   // Fill count
   const auto count = GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.count"_name, frame_index).map_memory());
   count.cast<u32>() = m_frame_updates[frame_index].top_index();
}

void RectangleRenderer::build_data_update(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("user_interface.rectangle.insertion"_name, sizeof(RectWriteData) * g_insertion_buffer_size);
   ctx.declare_staging_buffer("user_interface.rectangle.insertion.staging"_name, sizeof(RectWriteData) * g_insertion_buffer_size);
   ctx.declare_staging_buffer("user_interface.rectangle.insertion.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.rectangle.removal"_name, sizeof(RectCopyInfo) * g_removal_buffer_size);
   ctx.declare_staging_buffer("user_interface.rectangle.removal.staging"_name, sizeof(RectCopyInfo) * g_removal_buffer_size);
   ctx.declare_staging_buffer("user_interface.rectangle.removal.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.rectangle.draw_calls"_name, sizeof(DrawCall) * g_draw_call_buffer_size);

   ctx.declare_staging_buffer("user_interface.rectangle.insertion.indirect_buffer"_name, sizeof(Vector3u));
   ctx.declare_staging_buffer("user_interface.rectangle.removal.indirect_buffer"_name, sizeof(Vector3u));

   ctx.declare_staging_buffer("user_interface.rectangle.count"_name, sizeof(u32));

   // Copy buffers
   ctx.copy_buffer("user_interface.rectangle.insertion.staging"_name, "user_interface.rectangle.insertion"_name);
   ctx.copy_buffer("user_interface.rectangle.removal.staging"_name, "user_interface.rectangle.removal"_name);

   // Execute insertion
   ctx.bind_compute_shader("rectangle/insertion.cshader"_rc);
   ctx.bind_uniform_buffer(0, "user_interface.rectangle.insertion.count"_name);
   ctx.bind_storage_buffer(1, "user_interface.rectangle.insertion"_name);
   ctx.bind_storage_buffer(2, "user_interface.rectangle.draw_calls"_name);
   ctx.dispatch_indirect("user_interface.rectangle.insertion.indirect_buffer"_name);

   // Execute removal
   ctx.bind_compute_shader("rectangle/removal.cshader"_rc);
   ctx.bind_uniform_buffer(0, "user_interface.rectangle.removal.count"_name);
   ctx.bind_storage_buffer(1, "user_interface.rectangle.removal"_name);
   ctx.bind_storage_buffer(2, "user_interface.rectangle.draw_calls"_name);
   ctx.dispatch_indirect("user_interface.rectangle.removal.indirect_buffer"_name);

   ctx.export_buffer("user_interface.rectangle.draw_calls"_name, graphics_api::PipelineStage::VertexShader,
                     graphics_api::BufferAccess::ShaderRead,
                     graphics_api::BufferUsage::StorageBuffer | graphics_api::BufferUsage::Indirect);
   ctx.export_buffer("user_interface.rectangle.count"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
}

void RectangleRenderer::build_render_ui(render_core::BuildContext& ctx)
{
   ctx.bind_vertex_shader("rectangle/render.vshader"_rc);
   ctx.bind_uniform_buffer(0, "ui.viewport_info"_external);
   ctx.bind_storage_buffer(1, "user_interface.rectangle.draw_calls"_external);

   ctx.bind_fragment_shader("rectangle/render.fshader"_rc);

   ctx.draw_indirect_with_count("user_interface.rectangle.draw_calls"_external, "user_interface.rectangle.count"_external,
                                g_draw_call_buffer_size, sizeof(DrawCall));
}

}// namespace triglav::renderer::ui
