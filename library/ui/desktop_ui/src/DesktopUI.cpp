#include "DesktopUI.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

ThemeProperties ThemeProperties::get_default()
{
   return {
      .base_typeface = "lato/light.typeface"_rc,
      .base_font_size = 16,
      .background_color_darker = {0.094f, 0.094f, 0.094f, 1.0f},
      .background_color_brighter = {0.12f, 0.12f, 0.12f, 1.0f},
      .active_color = {0.16f, 0.16f, 0.16f, 1.0f},
      .foreground_color = {1.0f, 1.0f, 1.0f, 1.0f},
      .accent_color = {0.0f, 0.47f, 0.84f, 1.0f},

      .button =
         {
            .bg_color = {0.0f, 0.105f, 1.0f, 1.0f},
            .bg_hover_color = {0.035f, 0.22f, 1.0f, 1.0f},
            .bg_pressed_color = {0.0f, 0.015f, 0.48f, 1.0f},
            .font_size = 15,
         },

      .text_input =
         {
            .bg_inactive = {0.12f, 0.12f, 0.12f, 1.0f},
            .bg_active = {0.15f, 0.15f, 0.15f, 1.0f},
            .bg_hover = {0.16f, 0.16f, 0.16f, 1.0f},
         },

      .dropdown =
         {
            .bg = {0.12f, 0.12f, 0.12f, 1.0f},
            .bg_hover = {0.16f, 0.16f, 0.16f, 1.0f},
            .border = {0.2f, 0.2f, 0.2f, 1.0f},
            .border_width = 1.0f,
         },
      .checkbox =
         {
            .padding = 4.0f,
         },
   };
}

DesktopUIManager::DesktopUIManager(ThemeProperties properties, desktop::ISurface& surface, PopupManager& dialogManager) :
    m_properties(std::move(properties)),
    m_surface(surface),
    m_popupManager(dialogManager)
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

PopupManager& DesktopUIManager::popup_manager() const
{
   return m_popupManager;
}

}// namespace triglav::desktop_ui