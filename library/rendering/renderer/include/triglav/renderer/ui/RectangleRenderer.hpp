#pragma once

#include "triglav/Name.hpp"
#include "triglav/UpdateList.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <map>
#include <mutex>

namespace triglav::render_core {
class BuildContext;
class JobGraph;
}// namespace triglav::render_core

namespace triglav::renderer::ui {

struct RectPrimitive
{
   Vector4 dimensions;
   Vector4 border_radius;
   Vector4 border_color;
   Vector4 background_color;
   Vector4 cropping_mask;
   float border_width;
   u32 padding[3];
};

static_assert(sizeof(RectPrimitive) % 16 == 0);

struct RectWriteData
{
   RectPrimitive primitive;
   u32 dst_index;
   u32 padding[3];
};

static_assert(sizeof(RectWriteData) % 16 == 0);

struct RectCopyInfo
{
   u32 src_id;
   u32 dst_id;
};

class RectangleRenderer
{
 public:
   using Self = RectangleRenderer;

   RectangleRenderer(ui_core::Viewport& viewport);

   void on_added_rectangle(ui_core::RectId rect_id, const ui_core::Rectangle& rect);
   void on_updated_rectangle(ui_core::RectId rect_id, const ui_core::Rectangle& rect);
   void on_removed_rectangle(ui_core::RectId rect_id);

   void set_object(u32 index, const RectPrimitive& prim);
   void move_object(u32 src, u32 dst);

   void prepare_frame(render_core::JobGraph& graph, u32 frame_index);
   void build_data_update(render_core::BuildContext& ctx) const;
   void build_render_ui(render_core::BuildContext& ctx);

 private:
   std::array<UpdateList<ui_core::RectId, RectPrimitive>, render_core::FRAMES_IN_FLIGHT_COUNT> m_frame_updates;

   RectWriteData* m_staging_insertions;
   u32 m_staging_insertions_top{};
   RectCopyInfo* m_staging_removals;
   u32 m_staging_removals_top{};

   // std::map<Name, RectangleData> m_rectangles;
   std::mutex m_rect_update_mtx;

   TG_SINK(ui_core::Viewport, OnAddedRectangle);
   TG_SINK(ui_core::Viewport, OnUpdatedRectangle);
   TG_SINK(ui_core::Viewport, OnRemovedRectangle);
};

}// namespace triglav::renderer::ui
