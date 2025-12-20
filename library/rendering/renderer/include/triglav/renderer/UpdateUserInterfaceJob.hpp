#pragma once

#include "ui/RectangleRenderer.hpp"
#include "ui/SpriteRenderer.hpp"
#include "ui/TextRenderer.hpp"

#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/ui_core/Viewport.hpp"

namespace triglav::resource {
class ResourceManager;
}

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

   explicit UpdateUserInterfaceJob(graphics_api::Device& device, render_core::GlyphCache& glyph_cache, ui_core::Viewport& viewport,
                                   resource::ResourceManager& resource_manager, render_core::IRenderer& renderer);

   void build_job(render_core::BuildContext& ctx) const;
   void prepare_frame(render_core::JobGraph& graph, u32 frame_index);
   void render_ui(render_core::BuildContext& ctx);

 private:
   graphics_api::Device& m_device;
   ui_core::Viewport& m_viewport;
   ui::RectangleRenderer m_rectangle_renderer;
   ui::SpriteRenderer m_sprite_renderer;
   ui::TextRenderer m_text_renderer;
};

}// namespace triglav::renderer
