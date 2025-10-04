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
   Vector4 borderRadius;
   Vector4 borderColor;
   Vector4 backgroundColor;
   Vector4 croppingMask;
   float borderWidth;
   u32 padding[3];
};

static_assert(sizeof(RectPrimitive) % 16 == 0);

struct RectWriteData
{
   RectPrimitive primitive;
   u32 dstIndex;
   u32 padding[3];
};

static_assert(sizeof(RectWriteData) % 16 == 0);

struct RectCopyInfo
{
   u32 srcID;
   u32 dstID;
};

// struct RectangleData
// {
//    graphics_api::Buffer vsUbo;
//    graphics_api::Buffer fsUbo;
//    graphics_api::Buffer vertexBuffer;
// };

class RectangleRenderer
{
 public:
   using Self = RectangleRenderer;

   RectangleRenderer(graphics_api::Device& device, ui_core::Viewport& viewport);

   void on_added_rectangle(ui_core::RectId rectId, const ui_core::Rectangle& rect);
   void on_updated_rectangle(ui_core::RectId rectId, const ui_core::Rectangle& rect);
   void on_removed_rectangle(ui_core::RectId rectId);

   void add_insertion(u32 index, const RectPrimitive& prim);
   void add_removal(u32 src, u32 dst);

   void prepare_frame(render_core::JobGraph& graph, u32 frameIndex);
   void build_data_update(render_core::BuildContext& ctx) const;
   void build_render_ui(render_core::BuildContext& ctx);

 private:
   graphics_api::Device& m_device;
   ui_core::Viewport& m_viewport;
   std::array<UpdateList<ui_core::RectId, RectPrimitive>, render_core::FRAMES_IN_FLIGHT_COUNT> m_frameUpdates;

   RectWriteData* m_stagingInsertions;
   u32 m_stagingInsertionsTop{};
   RectCopyInfo* m_stagingRemovals;
   u32 m_stagingRemovalsTop{};

   // std::map<Name, RectangleData> m_rectangles;
   std::mutex m_rectUpdateMtx;

   TG_SINK(ui_core::Viewport, OnAddedRectangle);
   TG_SINK(ui_core::Viewport, OnUpdatedRectangle);
   TG_SINK(ui_core::Viewport, OnRemovedRectangle);
};

}// namespace triglav::renderer::ui
