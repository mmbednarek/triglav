#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/ui_core/Context.hpp"

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
      i32 font_size;
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

   // Tab View
   struct
   {
      i32 font_size;
   } tab_view;

   // Tree View
   struct
   {
      int font_size;
   } tree_view;

   static ThemeProperties get_default();
};

#define TG_THEME_VAL(name) this->desktop_context().properties().name

constexpr TextureName ICON_ATLAS = make_rc_name("engine/texture/ui_atlas.tex");

enum class AtlasIcon
{
   ArrowDown,
   ArrowUp,
   ArrowLeft,
   ArrowRight,
   GrayCross,
   Cross,
   MaxValue,
};

constexpr std::array ATLAS_ICONS = {
   // ArrowDown
   Vector4{0 * 64, 0 * 64, 64, 64},
   // ArrowUp
   Vector4{1 * 64, 0 * 64, 64, 64},
   // ArrowLeft
   Vector4{2 * 64, 0 * 64, 64, 64},
   // ArrowRight
   Vector4{3 * 64, 0 * 64, 64, 64},
   // GrayCross
   Vector4{0 * 64, 1 * 64, 64, 64},
   // Cross
   Vector4{1 * 64, 1 * 64, 64, 64},
};

constexpr Vector4 icon_region(const AtlasIcon icon)
{
   return ATLAS_ICONS[static_cast<u32>(icon)];
}

class DesktopContext : public ui_core::Context
{
 public:
   DesktopContext(ui_core::Viewport& viewport, render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager,
                  ThemeProperties properties, desktop::ISurface& surface, PopupManager& dialog_manager);
   DesktopContext(ui_core::Context& ctx, ThemeProperties properties, desktop::ISurface& surface, PopupManager& dialog_manager);

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
