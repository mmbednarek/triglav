#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

namespace triglav::desktop {
class ISurface;
}

namespace triglav::desktop_ui {

class DialogManager;

struct ThemeProperties
{
   // Core
   TypefaceName base_typeface;
   Vector4 background_color;
   Vector4 foreground_color;
   Vector4 accent_color;

   // Button
   Vector4 button_bg_color;
   Vector4 button_bg_hover_color;
   Vector4 button_bg_pressed_color;
   i32 button_font_size;

   // Text input
   Vector4 text_input_bg_inactive;
   Vector4 text_input_bg_active;
   Vector4 text_input_bg_hover;

   // Dropdown menu
   Vector4 dropdown_bg;
   Vector4 dropdown_bg_hover;
   Vector4 dropdown_border;
   float dropdown_border_width;
};

class DesktopUIManager
{
 public:
   DesktopUIManager(ThemeProperties properties, desktop::ISurface& surface, DialogManager& dialogManager);

   [[nodiscard]] const ThemeProperties& properties() const;
   [[nodiscard]] const desktop::ISurface& surface() const;
   [[nodiscard]] desktop::ISurface& surface();
   [[nodiscard]] DialogManager& dialog_manager() const;

 private:
   ThemeProperties m_properties;
   desktop::ISurface& m_surface;
   DialogManager& m_dialogManager;
};

}// namespace triglav::desktop_ui
