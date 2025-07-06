#include "DesktopUI.hpp"

namespace triglav::desktop_ui {

DesktopUIManager::DesktopUIManager(ThemeProperties properties, desktop::ISurface& surface, DialogManager& dialogManager) :
    m_properties(std::move(properties)),
    m_surface(surface),
    m_dialogManager(dialogManager)
{
}

const ThemeProperties& DesktopUIManager::properties() const
{
   return m_properties;
}

const desktop::ISurface& DesktopUIManager::surface() const
{
   return m_surface;
}

desktop::ISurface& DesktopUIManager::surface()
{
   return m_surface;
}

DialogManager& DesktopUIManager::dialog_manager() const
{
   return m_dialogManager;
}

}// namespace triglav::desktop_ui