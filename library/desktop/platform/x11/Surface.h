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

   void dispatch_key_press(KeyCode code) const;
   void dispatch_key_release(KeyCode code) const;
   void dispatch_button_press(uint32_t code) const;
   void dispatch_button_release(uint32_t code) const;
   void dispatch_mouse_move(int x, int y) const;
   void dispatch_mouse_relative_move(float x, float y) const;
   void dispatch_close() const;
   void tick() const;
   void set_cursor_icon(CursorIcon icon) override;

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
};

}// namespace triglav::desktop::x11