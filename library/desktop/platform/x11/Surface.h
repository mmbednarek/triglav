#pragma once

#include <X11/Xlib.h>
#include <vector>

#include "triglav/desktop/ISurface.hpp"

namespace triglav::desktop::x11 {

class Surface final : public ISurface
{
 public:
   explicit Surface(::Display* display, Window window, Dimension dimension);
   ~Surface() override;

   void lock_cursor() override;
   void unlock_cursor() override;
   void hide_cursor() const override;
   void internal_close();
   [[nodiscard]] bool is_cursor_locked() const override;
   [[nodiscard]] Dimension dimension() const override;

   void dispatch_key_press(const XEvent& event) const;
   void dispatch_key_release(const XEvent& event) const;
   void dispatch_button_press(const XEvent& event) const;
   void dispatch_button_release(const XEvent& event) const;
   void dispatch_mouse_move(const XEvent& event) const;
   void dispatch_mouse_relative_move(Vector2 diff) const;
   void dispatch_close() const;
   void tick() const;
   void set_cursor_icon(CursorIcon icon) override;
   void set_keyboard_input_mode(KeyboardInputModeFlags mode) override;

   [[nodiscard]] constexpr ::Display* display() const
   {
      return m_display;
   }

   [[nodiscard]] constexpr Window window() const
   {
      return m_window;
   }


 private:
   ::Display* m_display;
   Window m_window;
   Dimension m_dimension;
   ::Cursor m_currentCursor{};
   bool m_isCursorLocked{false};
   KeyboardInputModeFlags m_keyboardInputMode{KeyboardInputMode::Direct};
   XIM m_xim{};
   XIC m_xic{};
};

}// namespace triglav::desktop::x11