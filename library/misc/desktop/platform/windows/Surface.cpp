#include "Surface.hpp"

#include "Display.hpp"

#include "triglav/Format.hpp"
#include "triglav/String.hpp"

#include <windowsx.h>

namespace triglav::desktop {

using namespace string_literals;

namespace {

Key translate_key(const WPARAM key_code)
{
   switch (key_code) {
   case 'W':
      return Key::W;
   case 'A':
      return Key::A;
   case 'S':
      return Key::S;
   case 'D':
      return Key::D;
   case 'Q':
      return Key::Q;
   case 'E':
      return Key::E;
   case 'Z':
      return Key::Z;
   case 'Y':
      return Key::Y;
   case VK_BACK:
      return Key::Backspace;
   case VK_SPACE:
      return Key::Space;
   case VK_CONTROL:
      return Key::Control;
   case VK_SHIFT:
      return Key::Shift;
   case VK_MENU:
      return Key::Alt;
   case VK_RETURN:
      return Key::Enter;
   case VK_TAB:
      return Key::Tab;
   case VK_UP:
      return Key::UpArrow;
   case VK_DOWN:
      return Key::DownArrow;
   case VK_LEFT:
      return Key::LeftArrow;
   case VK_RIGHT:
      return Key::RightArrow;
   case VK_F1:
      return Key::F1;
   case VK_F2:
      return Key::F2;
   case VK_F3:
      return Key::F3;
   case VK_F4:
      return Key::F4;
   case VK_F5:
      return Key::F5;
   case VK_F6:
      return Key::F6;
   case VK_F7:
      return Key::F7;
   case VK_F8:
      return Key::F8;
   case VK_F9:
      return Key::F9;
   case VK_F10:
      return Key::F10;
   case VK_F11:
      return Key::F11;
   case VK_F12:
      return Key::F12;
   default:
      break;
   }

   return Key::Unknown;
}

DWORD map_window_attributes_to_ex_style(const WindowAttributeFlags flags, const bool is_popup)
{
   DWORD result{};
   if (flags & WindowAttribute::TopMost) {
      result |= WS_EX_TOPMOST;
   }
   if (flags & WindowAttribute::ShowDecorations) {
      result |= WS_EX_CLIENTEDGE;
   }
   if (is_popup) {
      result |= WS_EX_TOOLWINDOW;
   }

   return result;
}

DWORD map_window_attributes_to_style(const WindowAttributeFlags flags, const bool is_popup)
{
   DWORD result{};

   if (flags & WindowAttribute::ShowDecorations) {
      result |= WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
   }
   if (flags & WindowAttribute::Resizeable) {
      result |= WS_THICKFRAME;
   }
   if (is_popup) {
      result |= WS_POPUP;
   }
   return result;
}

int map_window_attributes_to_x_position(const WindowAttributeFlags flags, const Dimension& dimension)
{
   if (flags & WindowAttribute::AlignCenter) {
      auto screen_width = GetSystemMetrics(SM_CXSCREEN);
      return screen_width / 2 - dimension.x / 2;
   }
   return CW_USEDEFAULT;
}

int map_window_attributes_to_y_position(const WindowAttributeFlags flags, const Dimension& dimension)
{
   if (flags & WindowAttribute::AlignCenter) {
      auto screen_height = GetSystemMetrics(SM_CYSCREEN);
      return screen_height / 2 - dimension.y / 2;
   }
   return CW_USEDEFAULT;
}

HWND create_window(const HINSTANCE instance, const StringView title, const Vector2i dimension, const WindowAttributeFlags flags,
                   void* lp_param, const bool is_popup)
{
   const auto rune_count = title.rune_count();
   std::vector<wchar_t> w_char_title(rune_count + 1);
   std::transform(title.begin(), title.end(), w_char_title.begin(), [](const Rune r) { return static_cast<wchar_t>(r); });
   w_char_title[rune_count] = 0;

   const auto style = map_window_attributes_to_style(flags, is_popup);
   const auto style_ex = map_window_attributes_to_ex_style(flags, is_popup);

   RECT rect{
      /*left*/ 0,
      /*top*/ 0,
      /*right*/ dimension.x,
      /*botton*/ dimension.y,
   };
   if (!AdjustWindowRectEx(&rect, style, false, style_ex)) {
      log_message(LogLevel::Error, StringView{"WindowsSurface"}, "failed to calculate window size");
      return nullptr;
   }

   Vector2i client_dim{rect.right - rect.left, rect.bottom - rect.top};

   return CreateWindowExW(style_ex, g_window_class_name, w_char_title.data(), style, map_window_attributes_to_x_position(flags, dimension),
                          map_window_attributes_to_y_position(flags, dimension), client_dim.x, client_dim.y, nullptr, nullptr, instance,
                          lp_param);
}

}// namespace

Surface::Surface(Display& display, const StringView title, const Vector2i dimension, const WindowAttributeFlags flags,
                 const bool is_popup) :
    m_display(display),
    m_dimension(dimension),
    m_window_handle(create_window(display.winapi_instance(), title, dimension, flags, this, is_popup))
{
   ShowWindow(m_window_handle, SW_NORMAL);
   UpdateWindow(m_window_handle);
}

Surface::~Surface()
{
   DestroyWindow(m_window_handle);
}

void Surface::lock_cursor()
{
   RECT rect{};
   GetWindowRect(m_window_handle, &rect);
   SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
   SetCapture(m_window_handle);
}

void Surface::unlock_cursor()
{
   ReleaseCapture();
   ShowCursor(true);
}

void Surface::hide_cursor() const
{
   ShowCursor(false);
}

bool Surface::is_cursor_locked() const
{
   return GetCapture() == m_window_handle;
}

Dimension Surface::dimension() const
{
   return m_dimension;
}

void Surface::set_keyboard_input_mode(const KeyboardInputModeFlags mode)
{
   m_keyboard_input_mode = mode;
}

HINSTANCE Surface::winapi_instance() const
{
   return m_display.winapi_instance();
}

HWND Surface::winapi_window_handle() const
{
   return m_window_handle;
}

void Surface::on_key_down(const WPARAM key_code) const
{
   event_OnKeyIsPressed.publish(translate_key(key_code));
}

void Surface::on_key_up(WPARAM key_code) const
{
   event_OnKeyIsReleased.publish(translate_key(key_code));
}

void Surface::on_button_down(MouseButton button) const
{
   if (m_current_cursor != nullptr) {
      SetCursor(m_current_cursor);
   }
   event_OnMouseButtonIsPressed.publish(button);
}

void Surface::on_button_up(MouseButton button) const
{
   event_OnMouseButtonIsReleased.publish(button);
}

void Surface::on_mouse_move(const int x, const int y)
{
   if (m_current_cursor != nullptr) {
      SetCursor(m_current_cursor);
   }

   if (this->is_cursor_locked()) {
      POINT point{};
      GetCursorPos(&point);

      RECT rect{};
      GetWindowRect(m_window_handle, &rect);
      const auto origin_x = (rect.left + rect.right) / 2;
      const auto origin_y = (rect.top + rect.bottom) / 2;

      const int diff_x = point.x - origin_x;
      const int diff_y = point.y - origin_y;

      SetCursorPos(origin_x, origin_y);

      event_OnMouseRelativeMove.publish(Vector2{0.5f * static_cast<float>(diff_x), 0.5f * static_cast<float>(diff_y)});
   } else {
      event_OnMouseMove.publish(Vector2{static_cast<float>(x), static_cast<float>(y)});
   }
}

void Surface::on_close() const
{
   event_OnClose.publish();
}

void Surface::on_resize(const short x, const short y)
{
   m_dimension = Dimension{x, y};
   event_OnResize.publish(Vector2i{static_cast<float>(x), static_cast<float>(y)});
}

void Surface::on_rune(const Rune rune) const
{
   if (m_keyboard_input_mode & KeyboardInputMode::Text) {
      event_OnTextInput.publish(rune);
   }
}

void Surface::on_set_focus(const bool has_focus)
{
   if (has_focus) {
      POINT point;
      if (GetCursorPos(&point) && ScreenToClient(m_window_handle, &point)) {
         event_OnMouseEnter.publish(Vector2{static_cast<float>(point.x), static_cast<float>(point.y)});
      }
   } else {
      event_OnMouseLeave.publish();
   }
}

void Surface::on_mouse_wheel(int delta)
{
   if (delta != 0) {
      event_OnMouseWheelTurn.publish(-static_cast<float>(delta) / 30.0f);
   }
}

LRESULT Surface::handle_window_event(const UINT msg, const WPARAM w_param, const LPARAM l_param)
{
   switch (msg) {
   case WM_CLOSE: {
      this->on_close();
      break;
   }
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   case WM_KEYDOWN: {
      this->on_key_down(w_param);
      break;
   }
   case WM_KEYUP: {
      this->on_key_up(w_param);
      break;
   }
   case WM_LBUTTONDOWN: {
      this->on_button_down(MouseButton::Left);
      break;
   }
   case WM_MBUTTONDOWN: {
      this->on_button_down(MouseButton::Middle);
      break;
   }
   case WM_RBUTTONDOWN: {
      this->on_button_down(MouseButton::Right);
      break;
   }
   case WM_LBUTTONUP: {
      this->on_button_up(MouseButton::Left);
      break;
   }
   case WM_MBUTTONUP: {
      this->on_button_up(MouseButton::Middle);
      break;
   }
   case WM_RBUTTONUP: {
      this->on_button_up(MouseButton::Right);
      break;
   }
   case WM_MOUSEMOVE: {
      this->on_mouse_move(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
      break;
   }
   case WM_SIZE: {
      this->on_resize(LOWORD(l_param), HIWORD(l_param));
      break;
   }
   case WM_CHAR: {
      this->on_rune(static_cast<Rune>(w_param));
      break;
   }
   case WM_INPUT: {
      log_info("WM_INPUT event!");
      break;
   }
   case WM_SETFOCUS: {
      this->on_set_focus(true);
      break;
   }
   case WM_KILLFOCUS: {
      this->on_set_focus(false);
      break;
   }
   case WM_MOUSEWHEEL: {
      const auto delta = GET_WHEEL_DELTA_WPARAM(w_param);
      this->on_mouse_wheel(delta);
      break;
   }
   default:
      return DefWindowProcW(m_window_handle, msg, w_param, l_param);
   }

   return 0;
}

void Surface::set_cursor_icon(const CursorIcon icon)
{
   if (m_current_cursor != nullptr) {
      DestroyCursor(m_current_cursor);
   }

   switch (icon) {
   case CursorIcon::Arrow:
      m_current_cursor = LoadCursor(nullptr, IDC_ARROW);
      break;
   case CursorIcon::Hand:
      m_current_cursor = LoadCursor(nullptr, IDC_HAND);
      break;
   case CursorIcon::Move:
      m_current_cursor = LoadCursor(nullptr, IDC_SIZEALL);
      break;
   case CursorIcon::Wait:
      m_current_cursor = LoadCursor(nullptr, IDC_WAIT);
      break;
   case CursorIcon::Edit:
      m_current_cursor = LoadCursor(nullptr, IDC_IBEAM);
      break;
   case CursorIcon::ResizeHorizontal:
      m_current_cursor = LoadCursor(nullptr, IDC_SIZEWE);
      break;
   case CursorIcon::ResizeVertical:
      m_current_cursor = LoadCursor(nullptr, IDC_SIZENS);
      break;
   default:
      m_current_cursor = nullptr;
      return;
   }

   SetCursor(m_current_cursor);
   ShowCursor(true);
}

std::shared_ptr<ISurface> Surface::create_popup(const Vector2u dimensions, const Vector2 offset, const WindowAttributeFlags flags)
{
   auto popup_surface = std::make_shared<Surface>(m_display, "popup"_strv, dimensions, flags, true);

   RECT parent_rect;
   GetClientRect(m_window_handle, &parent_rect);
   ClientToScreen(m_window_handle, reinterpret_cast<LPPOINT>(&parent_rect.left));

   SetWindowPos(popup_surface->m_window_handle, HWND_TOP, parent_rect.left + static_cast<int>(offset.x),
                parent_rect.top + static_cast<int>(offset.y), 0, 0, SWP_NOSIZE);

   return popup_surface;
}

Display& Surface::display()
{
   return m_display;
}

}// namespace triglav::desktop
