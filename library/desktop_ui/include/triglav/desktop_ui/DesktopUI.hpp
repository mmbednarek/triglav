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
   Color background_color;
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
};

#define TG_THEME_VAL(name) m_state.manager->properties().name

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
