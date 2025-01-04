#pragma once

#include "ISurface.hpp"

#include <memory>
#include <windows.h>

namespace triglav::desktop {

constexpr auto g_windowClassName = "TRIGLAV_WINDOW";

class Surface : public ISurface, std::enable_shared_from_this<Surface>
{
 public:
   Surface(HINSTANCE instance, Dimension dimension, WindowAttributeFlags flags);
   ~Surface() override;

   void lock_cursor() override;
   void unlock_cursor() override;
   void hide_cursor() const override;
   void add_event_listener(ISurfaceEventListener* eventListener) override;
   [[nodiscard]] bool is_cursor_locked() const override;
   [[nodiscard]] Dimension dimension() const override;

   [[nodiscard]] HINSTANCE winapi_instance() const;
   [[nodiscard]] HWND winapi_window_handle() const;
   LRESULT handle_window_event(UINT msg, WPARAM wParam, LPARAM lParam);

 private:
   void on_key_down(WPARAM keyCode) const;
   void on_key_up(WPARAM keyCode) const;
   void on_button_down(MouseButton button) const;
   void on_button_up(MouseButton button) const;
   void on_mouse_move(int x, int y) const;
   void on_close() const;
   void on_resize(short x, short y);

   HINSTANCE m_instance;
   Dimension m_dimension;
   HWND m_windowHandle;
   ISurfaceEventListener* m_eventListener{};
};

}// namespace triglav::desktop