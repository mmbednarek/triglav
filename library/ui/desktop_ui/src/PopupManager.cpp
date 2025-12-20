#include "PopupManager.hpp"

namespace triglav::desktop_ui {

PopupManager::PopupManager(const graphics_api::Instance& instance, graphics_api::Device& device, render_core::GlyphCache& glyph_cache,
                           resource::ResourceManager& resource_manager, desktop::ISurface& root_surface, render_core::IRenderer& renderer) :
    m_instance(instance),
    m_device(device),
    m_glyph_cache(glyph_cache),
    m_resource_manager(resource_manager),
    m_root_surface(root_surface),
    m_renderer(renderer)
{
}

Dialog& PopupManager::create_popup_dialog(const Vector2i offset, const Vector2u dimensions)
{
   std::unique_lock lk{m_popup_mtx};
   const auto& popup = m_popups.emplace_back(
      std::make_unique<Dialog>(m_instance, m_device, m_root_surface, m_glyph_cache, m_resource_manager, m_renderer, dimensions, offset));
   return *popup;
}

void PopupManager::tick()
{
   std::unique_lock lk{m_popup_mtx};

   if (!m_popups_to_erase.empty()) {
      m_device.await_all();

      for (const auto& dialog : m_popups_to_erase) {
         auto it = std::ranges::find_if(m_popups, [dialog](const auto& popup) { return popup.get() == dialog; });
         m_popups.erase(it);
      }
      m_popups_to_erase.clear();
   }

   for (const auto& popup : m_popups) {
      popup->update();
   }
}

void PopupManager::close_popup(Dialog* dialog)
{
   assert(dialog != nullptr);
   dialog->uninitialize();

   std::unique_lock lk{m_popup_mtx};
   m_popups_to_erase.emplace_back(dialog);
}

desktop::ISurface& PopupManager::root_surface() const
{
   return m_root_surface;
}

}// namespace triglav::desktop_ui
