#pragma once

#include "ISurface.hpp"

#include <memory>
#include <string_view>
#include <windows.h>

namespace triglav::desktop {

constexpr auto g_windowClassName = "TRIGLAV_WINDOW";

class Surface : public ISurface, std::enable_shared_from_this<Surface>
{
 public:
   Surface(HINSTANCE instance, std::string_view title, Vector2i dimension, WindowAttributeFlags flags);
   ~Surface() override;

   void lock_cursor() override;
   void unlock_cursor() override;
   void hide_cursor() const override;
   [[nodiscard]] bool is_cursor_locked() const override;
   [[nodiscard]] Vector2i dimension() const override;

   [[nodiscard]] HINSTANCE winapi_instance() const;
   [[nodiscard]] HWND winapi_window_handle() const;
   LRESULT handle_window_event(UINT msg, WPARAM wParam, LPARAM lParam);
   void set_cursor_icon(CursorIcon icon) override;

 private:
   void on_key_down(WPARAM keyCode) const;
   void on_key_up(WPARAM keyCode) const;
   void on_button_down(MouseButton button) const;
   void on_button_up(MouseButton button) const;
   void on_mouse_move(int x, int y) const;
   void on_close() const;
   void on_resize(short x, short y);

   HINSTANCE m_instance;
   Vector2i m_dimension;
   HWND m_windowHandle;
   HCURSOR m_currentCursor{};
};

}// namespace triglav::desktop