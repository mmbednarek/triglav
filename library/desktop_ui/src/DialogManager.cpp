#include "DialogManager.hpp"

namespace triglav::desktop_ui {

DialogManager::DialogManager(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
                             render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager,
                             const Vector2u rootDimensions) :
    m_instance(instance),
    m_device(device),
    m_display(display),
    m_glyphCache(glyphCache),
    m_resourceManager(resourceManager),
    m_root(std::make_unique<Dialog>(m_instance, m_device, m_display, m_glyphCache, m_resourceManager, rootDimensions))
{
}

Dialog& DialogManager::root() const
{
   return *m_root;
}

Dialog& DialogManager::create_popup_dialog(const Vector2i offset, const Vector2u dimensions)
{
   std::unique_lock lk{m_popupMtx};
   const auto& popup = m_popups.emplace_back(
      std::make_unique<Dialog>(m_instance, m_device, m_root->surface(), m_glyphCache, m_resourceManager, dimensions, offset));
   return *popup;
}

bool DialogManager::should_close() const
{
   return m_root->should_close();
}

void DialogManager::tick()
{
   std::unique_lock lk{m_popupMtx};

   if (!m_popupsToEarse.empty()) {
      m_device.await_all();

      for (const auto& dialog : m_popupsToEarse) {
         auto it = std::ranges::find_if(m_popups, [dialog](const auto& popup) { return popup.get() == dialog; });
         m_popups.erase(it);
      }
      m_popupsToEarse.clear();
   }

   m_root->update();
   for (const auto& popup : m_popups) {
      popup->update();
   }
}

void DialogManager::close_popup(Dialog* dialog)
{
   assert(dialog != nullptr);
   dialog->uninitialize();

   std::unique_lock lk{m_popupMtx};
   m_popupsToEarse.emplace_back(dialog);
}

}// namespace triglav::desktop_ui
