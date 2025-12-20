#pragma once

#include "Dialog.hpp"

#include <mutex>
#include <vector>

namespace triglav::desktop_ui {

class PopupManager
{
 public:
   using Self = PopupManager;

   PopupManager(const graphics_api::Instance& instance, graphics_api::Device& device, render_core::GlyphCache& glyph_cache,
                resource::ResourceManager& resource_manager, desktop::ISurface& root_surface, render_core::IRenderer& renderer);

   [[nodiscard]] Dialog& create_popup_dialog(Vector2i offset, Vector2u dimensions);
   void tick();
   void close_popup(Dialog* dialog);
   desktop::ISurface& root_surface() const;

 private:
   const graphics_api::Instance& m_instance;
   graphics_api::Device& m_device;
   render_core::GlyphCache& m_glyph_cache;
   resource::ResourceManager& m_resource_manager;
   desktop::ISurface& m_root_surface;
   render_core::IRenderer& m_renderer;
   std::vector<std::unique_ptr<Dialog>> m_popups;
   std::vector<Dialog*> m_popups_to_erase;
   std::mutex m_popup_mtx;
};

}// namespace triglav::desktop_ui
