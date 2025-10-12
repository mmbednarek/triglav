#include "DesktopUI.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

ThemeProperties ThemeProperties::get_default()
{
   return {
      .base_typeface = "cantarell.typeface"_rc,
      .base_font_size = 15,
      .background_color = {0.05f, 0.05f, 0.05f, 1.0f},
      .active_color = {0.1f, 0.1f, 0.1f, 1.0f},
      .foreground_color = {1.0f, 1.0f, 1.0f, 1.0f},
      .accent_color = {0.0f, 0.105f, 1.0f, 1.0f},

      .button =
         {
            .bg_color = {0.0f, 0.105f, 1.0f, 1.0f},
            .bg_hover_color = {0.035f, 0.22f, 1.0f, 1.0f},
            .bg_pressed_color = {0.0f, 0.015f, 0.48f, 1.0f},
            .font_size = 15,
         },

      .text_input =
         {
            .bg_inactive = {0.03f, 0.03f, 0.03f, 1.0f},
            .bg_active = {0.01f, 0.01f, 0.01f, 1.0f},
            .bg_hover = {0.035f, 0.035f, 0.035f, 1.0f},
         },

      .dropdown =
         {
            .bg = {0.04f, 0.04f, 0.04f, 1.0f},
            .bg_hover = {0.05f, 0.05f, 0.05f, 1.0f},
            .border = {0.1f, 0.1f, 0.1f, 1.0f},
            .border_width = 1.0f,
         },
      .checkbox =
         {
            .padding = 4.0f,
         },
   };
}

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