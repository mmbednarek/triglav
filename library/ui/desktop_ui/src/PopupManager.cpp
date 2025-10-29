#include "PopupManager.hpp"

namespace triglav::desktop_ui {

PopupManager::PopupManager(const graphics_api::Instance& instance, graphics_api::Device& device, render_core::GlyphCache& glyphCache,
                           resource::ResourceManager& resourceManager, desktop::ISurface& rootSurface) :
    m_instance(instance),
    m_device(device),
    m_glyphCache(glyphCache),
    m_resourceManager(resourceManager),
    m_rootSurface(rootSurface)
{
}

Dialog& PopupManager::create_popup_dialog(const Vector2i offset, const Vector2u dimensions)
{
   std::unique_lock lk{m_popupMtx};
   const auto& popup = m_popups.emplace_back(
      std::make_unique<Dialog>(m_instance, m_device, m_rootSurface, m_glyphCache, m_resourceManager, dimensions, offset));
   return *popup;
}

void PopupManager::tick()
{
   std::unique_lock lk{m_popupMtx};

   if (!m_popupsToErase.empty()) {
      m_device.await_all();

      for (const auto& dialog : m_popupsToErase) {
         auto it = std::ranges::find_if(m_popups, [dialog](const auto& popup) { return popup.get() == dialog; });
         m_popups.erase(it);
      }
      m_popupsToErase.clear();
   }

   for (const auto& popup : m_popups) {
      popup->update();
   }
}

void PopupManager::close_popup(Dialog* dialog)
{
   assert(dialog != nullptr);
   dialog->uninitialize();

   std::unique_lock lk{m_popupMtx};
   m_popupsToErase.emplace_back(dialog);
}

desktop::ISurface& PopupManager::root_surface() const
{
   return m_rootSurface;
}

}// namespace triglav::desktop_ui
