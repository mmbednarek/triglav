#pragma once

#include "Dialog.hpp"

#include <vector>
#include <mutex>

namespace triglav::desktop_ui {

class DialogManager
{
 public:
   using Self = DialogManager;

   DialogManager(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
                 render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager, Vector2u rootDimensions);

   [[nodiscard]] Dialog& root() const;
   [[nodiscard]] Dialog& create_popup_dialog(Vector2i offset, Vector2u dimensions);
   [[nodiscard]] bool should_close() const;
   void tick();
   void close_popup(Dialog* dialog);

 private:
   const graphics_api::Instance& m_instance;
   graphics_api::Device& m_device;
   desktop::IDisplay& m_display;
   render_core::GlyphCache& m_glyphCache;
   resource::ResourceManager& m_resourceManager;
   std::unique_ptr<Dialog> m_root;
   std::vector<std::unique_ptr<Dialog>> m_popups;
   std::vector<Dialog*> m_popupsToEarse;
   std::mutex m_popupMtx;
};

}// namespace triglav::desktop_ui
