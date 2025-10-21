#pragma once

#include "Dialog.hpp"

#include <mutex>
#include <vector>

namespace triglav::desktop_ui {

class PopupManager
{
 public:
   using Self = PopupManager;

   PopupManager(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
                render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager, desktop::ISurface& rootSurface);

   [[nodiscard]] Dialog& create_popup_dialog(Vector2i offset, Vector2u dimensions);
   void tick();
   void close_popup(Dialog* dialog);
   desktop::ISurface& root_surface() const;

 private:
   const graphics_api::Instance& m_instance;
   graphics_api::Device& m_device;
   desktop::IDisplay& m_display;
   render_core::GlyphCache& m_glyphCache;
   resource::ResourceManager& m_resourceManager;
   desktop::ISurface& m_rootSurface;
   std::vector<std::unique_ptr<Dialog>> m_popups;
   std::vector<Dialog*> m_popupsToErase;
   std::mutex m_popupMtx;
};

}// namespace triglav::desktop_ui
