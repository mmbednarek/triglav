#pragma once

#include "Display.hpp"
#include "ISurface.hpp"
extern "C"
{
#include "api/XdgShell.h"
}

#include "triglav/Logging.hpp"

#include <optional>

namespace triglav::desktop::wayland {

class Display;

class Surface : public ISurface
{
   TG_DEFINE_LOG_CATEGORY(WaylandSurface)
   friend Display;

 public:
   Surface(Display& display, StringView title, Dimension dimension, WindowAttributeFlags attributes);
   Surface(Display& display, ISurface& parent_surface, Dimension dimension, Vector2 offset, WindowAttributeFlags attributes);

   ~Surface() override;

   void on_configure(uint32_t serial);
   void on_toplevel_configure(int32_t width, int32_t height, wl_array* states);
   void on_toplevel_close() const;
   void on_popup_configure(Vector2i position, Vector2i size);
   void on_popup_done();
   void on_popup_repositioned(u32 token);

   void lock_cursor() override;
   void unlock_cursor() override;
   void hide_cursor() const override;
   void tick();
   [[nodiscard]] bool is_cursor_locked() const override;
   [[nodiscard]] Dimension dimension() const override;
   void set_cursor_icon(CursorIcon icon) override;
   void set_keyboard_input_mode(KeyboardInputModeFlags mode) override;
   [[nodiscard]] std::shared_ptr<ISurface> create_popup(Vector2u dimensions, Vector2 offset, WindowAttributeFlags flags) override;

   [[nodiscard]] constexpr Display& display() const noexcept
   {
      return m_display;
   }

   [[nodiscard]] constexpr wl_surface* surface() const noexcept
   {
      return m_surface;
   }

 private:
   Display& m_display;
   wl_surface* m_surface{};
   xdg_surface* m_xdg_surface{};
   xdg_surface_listener m_surface_listener{};
   xdg_toplevel* m_top_level{};
   xdg_toplevel_listener m_top_level_listener{};
   xdg_popup_listener m_popup_listener{};
   zwp_locked_pointer_v1* m_locked_pointer{};
   zxdg_toplevel_decoration_v1* m_decoration{};
   xdg_popup* m_popup{};
   bool m_resize_ready = false;
   bool m_is_configured = false;
   int m_reposition_token{0};
   KeyboardInputModeFlags m_keyboard_input_mode{KeyboardInputMode::Direct};

   uint32_t m_pointer_serial{};

   Dimension m_dimension{};
   WindowAttributeFlags m_attributes{};
   std::optional<Dimension> m_pending_dimension{};
};

}// namespace triglav::desktop::wayland