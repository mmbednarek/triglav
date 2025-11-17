#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

namespace triglav::desktop {
class ISurface;
}

namespace triglav::desktop_ui {

class PopupManager;

struct ThemeProperties
{
   // Core
   TypefaceName base_typeface;
   i32 base_font_size;
   Color background_color_darker;
   Color background_color_brighter;
   Color active_color;
   Color foreground_color;
   Color accent_color;

   // Button
   struct
   {
      Color bg_color;
      Color bg_hover_color;
      Color bg_pressed_color;
      i32 font_size;
   } button;

   // Text input
   struct
   {
      Color bg_inactive;
      Color bg_active;
      Color bg_hover;
   } text_input;

   // Dropdown menu
   struct
   {
      Vector4 bg;
      Vector4 bg_hover;
      Vector4 border;
      float border_width;
   } dropdown;

   // Checkbox
   struct
   {
      float padding;
   } checkbox;

   static ThemeProperties get_default();
};

#define TG_THEME_VAL(name) m_state.manager->properties().name

class DesktopUIManager
{
 public:
   DesktopUIManager(ThemeProperties properties, desktop::ISurface& surface, PopupManager& dialog_manager);

   [[nodiscard]] const ThemeProperties& properties() const;
   [[nodiscard]] const desktop::ISurface& surface() const;
   [[nodiscard]] desktop::ISurface& surface();
   [[nodiscard]] PopupManager& popup_manager() const;

 private:
   ThemeProperties m_properties;
   desktop::ISurface& m_surface;
   PopupManager& m_popup_manager;
};

}// namespace triglav::desktop_ui
