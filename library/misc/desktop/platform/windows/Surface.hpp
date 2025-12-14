#pragma once

#include "ISurface.hpp"

#include "triglav/Logging.hpp"
#include "triglav/String.hpp"

#include <memory>
#include <string_view>
#include <windows.h>

namespace triglav::desktop {

constexpr auto g_window_class_name = L"TRIGLAV_WINDOW";

class Display;

class Surface : public ISurface, std::enable_shared_from_this<Surface>
{
   TG_DEFINE_LOG_CATEGORY(WindowsSurface)
 public:
   Surface(Display& display, StringView title, Vector2i dimension, WindowAttributeFlags flags, bool is_popup);
   ~Surface() override;

   void lock_cursor() override;
   void unlock_cursor() override;
   void hide_cursor() const override;
   [[nodiscard]] bool is_cursor_locked() const override;
   [[nodiscard]] Vector2i dimension() const override;
   void set_keyboard_input_mode(KeyboardInputModeFlags mode) override;

   [[nodiscard]] HINSTANCE winapi_instance() const;
   [[nodiscard]] HWND winapi_window_handle() const;
   LRESULT handle_window_event(UINT msg, WPARAM w_param, LPARAM l_param);
   void set_cursor_icon(CursorIcon icon) override;
   [[nodiscard]] std::shared_ptr<ISurface> create_popup(Vector2u dimensions, Vector2 offset, WindowAttributeFlags flags) override;
   ModifierFlags modifiers() const override;

   [[nodiscard]] Display& display();

 private:
   void on_key_down(WPARAM key_code) const;
   void on_key_up(WPARAM key_code) const;
   void on_button_down(MouseButton button) const;
   void on_button_up(MouseButton button) const;
   void on_mouse_move(int x, int y);
   void on_close() const;
   void on_resize(short x, short y);
   void on_rune(Rune rune) const;
   void on_set_focus(bool has_focus);
   void on_mouse_wheel(int delta);

   Display& m_display;
   Vector2i m_dimension;
   HWND m_window_handle;
   HCURSOR m_current_cursor{};
   KeyboardInputModeFlags m_keyboard_input_mode{};
};

}// namespace triglav::desktop