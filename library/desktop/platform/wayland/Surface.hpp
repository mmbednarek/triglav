#pragma once

#include "Display.hpp"
#include "ISurface.hpp"
extern "C"
{
#include "api/XdgShell.h"
}

#include <optional>

namespace triglav::desktop {

class Display;

class Surface : public ISurface
{
   friend Display;

 public:
   explicit Surface(Display& display, StringView title, Dimension dimension, WindowAttributeFlags attributes);
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
   void set_parent_surface(ISurface& other, Vector2 offset) override;

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
   xdg_surface* m_xdgSurface{};
   xdg_surface_listener m_surfaceListener{};
   xdg_toplevel* m_topLevel{};
   xdg_toplevel_listener m_topLevelListener{};
   xdg_popup_listener m_popupListener{};
   zwp_locked_pointer_v1* m_lockedPointer{};
   zxdg_toplevel_decoration_v1* m_decoration{};
   xdg_popup* m_popup{};
   bool m_resizeReady = false;
   bool m_isConfigured = false;
   int m_repositionToken{0};
   KeyboardInputModeFlags m_keyboardInputMode{KeyboardInputMode::Direct};

   uint32_t m_pointerSerial{};

   Dimension m_dimension{};
   WindowAttributeFlags m_attributes{};
   std::optional<Dimension> m_pendingDimension{};
};

}// namespace triglav::desktop