#include "DesktopUI.hpp"

namespace triglav::desktop_ui {

DesktopUIManager::DesktopUIManager(ThemeProperties properties, desktop::ISurface& surface) :
    m_properties(std::move(properties)),
    m_surface(surface)
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

}// namespace triglav::desktop_ui