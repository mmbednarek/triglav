#include "DesktopUI.hpp"

namespace triglav::desktop_ui {

DesktopUIManager::DesktopUIManager(ThemeProperties properties) :
    m_properties(std::move(properties))
{
}

const ThemeProperties& DesktopUIManager::properties() const
{
   return m_properties;
}

}// namespace triglav::desktop_ui