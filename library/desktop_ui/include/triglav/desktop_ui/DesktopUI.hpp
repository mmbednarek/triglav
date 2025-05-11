#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

namespace triglav::desktop_ui {

struct ThemeProperties
{
   Vector4 background_color;
   Vector4 foreground_color;
   Vector4 accent_color;
   Vector4 button_bg_color;
   Vector4 button_bg_hover_color;
   Vector4 button_bg_pressed_color;
   i32 button_font_size;
   TypefaceName base_typeface;
};

class DesktopUIManager
{
 public:
   explicit DesktopUIManager(ThemeProperties properties);

   [[nodiscard]] const ThemeProperties& properties() const;

 private:
   ThemeProperties m_properties;
};

}// namespace triglav::desktop_ui
