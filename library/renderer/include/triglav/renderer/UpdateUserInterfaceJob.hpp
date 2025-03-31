#pragma once

#include "ui/RectangleRenderer.hpp"
#include "ui/TextRenderer.hpp"

#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::render_core {
class BuildContext;
class JobGraph;
class GlyphCache;
}// namespace triglav::render_core

namespace triglav::renderer {

class UpdateUserInterfaceJob
{
 public:
   using Self = UpdateUserInterfaceJob;

   static constexpr auto JobName = make_name_id("job.update_user_interface");

   explicit UpdateUserInterfaceJob(graphics_api::Device& device, render_core::GlyphCache& glyphCache, ui_core::Viewport& viewport);

   void build_job(render_core::BuildContext& ctx) const;
   void prepare_frame(render_core::JobGraph& graph, u32 frameIndex);
   void render_ui(render_core::BuildContext& ctx);

 private:
   graphics_api::Device& m_device;
   ui::TextRenderer m_textRenderer;
   ui::RectangleRenderer m_rectangleRenderer;
};

}// namespace triglav::renderer
