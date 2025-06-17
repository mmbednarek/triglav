#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

namespace triglav::desktop {
class ISurface;
}

namespace triglav::desktop_ui {

struct ThemeProperties
{
   Vector4 background_color;
   Vector4 foreground_color;
   Vector4 accent_color;
   Vector4 button_bg_color;
   Vector4 button_bg_hover_color;
   Vector4 button_bg_pressed_color;
   Vector4 text_input_bg_inactive;
   Vector4 text_input_bg_active;
   Vector4 text_input_bg_hover;
   i32 button_font_size;
   TypefaceName base_typeface;
};

class DesktopUIManager
{
 public:
   DesktopUIManager(ThemeProperties properties, desktop::ISurface& surface);

   [[nodiscard]] const ThemeProperties& properties() const;
   [[nodiscard]] const desktop::ISurface& surface() const;
   [[nodiscard]] desktop::ISurface& surface();

 private:
   ThemeProperties m_properties;
   desktop::ISurface& m_surface;
};

}// namespace triglav::desktop_ui
