#include "DesktopUI.hpp"

#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

ThemeProperties ThemeProperties::get_default()
{
   return {
      .base_typeface = "engine/fonts/inter/light.typeface"_rc,
      .base_font_size = 15,
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
      .tab_view =
         {
            .font_size = 14,
         },
      .tree_view =
         {
            .font_size = 14,
         },
   };
}

DesktopContext::DesktopContext(ui_core::Viewport& viewport, render_core::GlyphCache& glyph_cache,
                               resource::ResourceManager& resource_manager, ThemeProperties properties, desktop::ISurface& surface,
                               PopupManager& dialog_manager) :
    ui_core::Context(viewport, glyph_cache, resource_manager),
    m_properties(properties),
    m_surface(surface),
    m_popup_manager(dialog_manager)
{
}


DesktopContext::DesktopContext(ui_core::Context& ctx, ThemeProperties properties, desktop::ISurface& surface,
                               PopupManager& dialog_manager) :
    ui_core::Context(ctx),
    m_properties(properties),
    m_surface(surface),
    m_popup_manager(dialog_manager)
{
}

const ThemeProperties& DesktopContext::properties() const
{
   return m_properties;
}

const desktop::ISurface& DesktopContext::surface() const
{
   return m_surface;
}

desktop::ISurface& DesktopContext::surface()
{
   return m_surface;
}

PopupManager& DesktopContext::popup_manager() const
{
   return m_popup_manager;
}

}// namespace triglav::desktop_ui