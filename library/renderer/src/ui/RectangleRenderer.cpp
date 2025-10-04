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
constexpr auto g_insertionBufferSize = 32;

// Staging removal buffer
// GPU removal buffer
constexpr auto g_removalBufferSize = 32;

// Draw call Buffer
constexpr auto g_drawCallBufferSize = 512;

constexpr auto g_csGroupSize = 256;

struct DrawCall
{
   u32 vertexCount;
   u32 instanceCount;
   u32 firstVertex;
   u32 firstInstance;

   RectPrimitive primitive;
};

static_assert(sizeof(DrawCall) % 16 == 0);

namespace {

RectPrimitive to_primitive(const ui_core::Rectangle& rect)
{
   return {
      .dimensions = rect.rect,
      .borderRadius = rect.borderRadius,
      .borderColor = rect.borderColor,
      .backgroundColor = rect.color,
      .croppingMask = rect.crop,
      .borderWidth = rect.borderWidth,
   };
}

}// namespace

RectangleRenderer::RectangleRenderer(graphics_api::Device& device, ui_core::Viewport& viewport) :
    m_device(device),
    m_viewport(viewport),
    TG_CONNECT(viewport, OnAddedRectangle, on_added_rectangle),
    TG_CONNECT(viewport, OnUpdatedRectangle, on_updated_rectangle),
    TG_CONNECT(viewport, OnRemovedRectangle, on_removed_rectangle)
{
}

void RectangleRenderer::on_added_rectangle(const RectId rectId, const ui_core::Rectangle& rect)
{
   this->on_updated_rectangle(rectId, rect);
}

void RectangleRenderer::on_updated_rectangle(const RectId rectId, const ui_core::Rectangle& rect)
{
   std::unique_lock lk{m_rectUpdateMtx};
   for (auto& updates : m_frameUpdates) {
      updates.add_or_update(rectId, to_primitive(rect));
   }
}

void RectangleRenderer::on_removed_rectangle(const RectId rectId)
{
   std::unique_lock lk{m_rectUpdateMtx};
   for (auto& updates : m_frameUpdates) {
      updates.remove(rectId);
   }
}

void RectangleRenderer::add_insertion(const u32 index, const RectPrimitive& prim)
{
   m_stagingInsertions[m_stagingInsertionsTop].dstIndex = index;
   m_stagingInsertions[m_stagingInsertionsTop].primitive = prim;
   m_stagingInsertionsTop++;
}

void RectangleRenderer::add_removal(const u32 src, const u32 dst)
{
   m_stagingRemovals[m_stagingRemovalsTop].srcID = src;
   m_stagingRemovals[m_stagingRemovalsTop].dstID = dst;
   m_stagingRemovalsTop++;
}

void RectangleRenderer::prepare_frame(render_core::JobGraph& graph, const u32 frameIndex)
{
   // Insertions
   const auto insertions = GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.insertion.staging"_name, frameIndex).map_memory());
   m_stagingInsertions = &insertions.cast<RectWriteData>();
   m_stagingInsertionsTop = 0;

   // Removals
   const auto removals = GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.removal.staging"_name, frameIndex).map_memory());
   m_stagingRemovals = &removals.cast<RectCopyInfo>();
   m_stagingRemovalsTop = 0;

   m_frameUpdates[frameIndex].write_to_buffers(*this);

   // Write calls
   const auto insertion_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.insertion.indirect_buffer"_name, frameIndex).map_memory());
   insertion_dims.cast<Vector3u>() = {divide_rounded_up(m_stagingInsertionsTop, g_csGroupSize), 1, 1};
   const auto insertion_count =
      GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.insertion.count"_name, frameIndex).map_memory());
   insertion_count.cast<u32>() = m_stagingInsertionsTop;

   const auto removal_dims =
      GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.removal.indirect_buffer"_name, frameIndex).map_memory());
   removal_dims.cast<Vector3u>() = {divide_rounded_up(m_stagingRemovalsTop, g_csGroupSize), 1, 1};
   const auto removal_count = GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.removal.count"_name, frameIndex).map_memory());
   removal_count.cast<u32>() = m_stagingRemovalsTop;

   // Fill count
   const auto count = GAPI_CHECK(graph.resources().buffer("user_interface.rectangle.count"_name, frameIndex).map_memory());
   count.cast<u32>() = m_frameUpdates[frameIndex].top_index();
}

void RectangleRenderer::build_data_update(render_core::BuildContext& ctx) const
{
   ctx.declare_buffer("user_interface.rectangle.insertion"_name, sizeof(RectWriteData) * g_insertionBufferSize);
   ctx.declare_staging_buffer("user_interface.rectangle.insertion.staging"_name, sizeof(RectWriteData) * g_insertionBufferSize);
   ctx.declare_staging_buffer("user_interface.rectangle.insertion.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.rectangle.removal"_name, sizeof(RectCopyInfo) * g_removalBufferSize);
   ctx.declare_staging_buffer("user_interface.rectangle.removal.staging"_name, sizeof(RectCopyInfo) * g_removalBufferSize);
   ctx.declare_staging_buffer("user_interface.rectangle.removal.count"_name, sizeof(u32));

   ctx.declare_buffer("user_interface.rectangle.draw_calls"_name, sizeof(DrawCall) * g_drawCallBufferSize);

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
                                g_drawCallBufferSize, sizeof(DrawCall));
}

}// namespace triglav::renderer::ui
