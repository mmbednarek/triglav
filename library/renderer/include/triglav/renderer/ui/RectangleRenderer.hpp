#pragma once

#include "triglav/Name.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <map>
#include <mutex>

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer::ui {

struct RectangleData
{
   graphics_api::Buffer vsUbo;
   graphics_api::Buffer fsUbo;
   graphics_api::Buffer vertexBuffer;
};

class RectangleRenderer
{
 public:
   using Self = RectangleRenderer;

   RectangleRenderer(graphics_api::Device& device, ui_core::Viewport& viewport);

   void on_added_rectangle(Name name, const ui_core::Rectangle& rect);
   void on_rectangle_change_dims(Name name, const ui_core::Rectangle& rect);
   void on_rectangle_change_color(Name name, const ui_core::Rectangle& rect);

   void build_render_ui(render_core::BuildContext& ctx);

 private:
   graphics_api::Device& m_device;
   ui_core::Viewport& m_viewport;

   std::map<Name, RectangleData> m_rectangles;
   std::mutex m_rectUpdateMtx;

   TG_SINK(ui_core::Viewport, OnAddedRectangle);
   TG_SINK(ui_core::Viewport, OnRectangleChangeDims);
   TG_SINK(ui_core::Viewport, OnRectangleChangeColor);
};

}// namespace triglav::renderer::ui
