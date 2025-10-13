#include "Surface.hpp"

#include "Display.hpp"

#include "triglav/Format.hpp"
#include "triglav/String.hpp"

#include <spdlog/spdlog.h>
#include <windowsx.h>

namespace triglav::desktop {

using namespace string_literals;

namespace {

Key translate_key(const WPARAM keyCode)
{
   switch (keyCode) {
   case 'W':
      return Key::W;
   case 'A':
      return Key::A;
   case 'S':
      return Key::S;
   case 'D':
      return Key::D;
   case VK_BACK:
      return Key::Backspace;
   case VK_SPACE:
      return Key::Space;
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

DWORD map_window_attributes_to_ex_style(const WindowAttributeFlags flags, const bool isPopup)
{
   DWORD result{};
   if (flags & WindowAttribute::TopMost) {
      result |= WS_EX_TOPMOST;
   }
   if (flags & WindowAttribute::ShowDecorations) {
      result |= WS_EX_CLIENTEDGE;
   }
   if (isPopup) {
      result |= WS_EX_TOOLWINDOW;
   }

   return result;
}

DWORD map_window_attributes_to_style(const WindowAttributeFlags flags, const bool isPopup)
{
   DWORD result{};

   if (flags & WindowAttribute::ShowDecorations) {
      result |= WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
   }
   if (flags & WindowAttribute::Resizeable) {
      result |= WS_THICKFRAME;
   }
   if (isPopup) {
      result |= WS_POPUP;
   }
   return result;
}

int map_window_attributes_to_x_position(const WindowAttributeFlags flags, const Dimension& dimension)
{
   if (flags & WindowAttribute::AlignCenter) {
      auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
      return screenWidth / 2 - dimension.x / 2;
   }
   return CW_USEDEFAULT;
}

int map_window_attributes_to_y_position(const WindowAttributeFlags flags, const Dimension& dimension)
{
   if (flags & WindowAttribute::AlignCenter) {
      auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
      return screenHeight / 2 - dimension.y / 2;
   }
   return CW_USEDEFAULT;
}

HWND create_window(const HINSTANCE instance, const StringView title, const Vector2i dimension, const WindowAttributeFlags flags,
                   void* lpParam, const bool isPopup)
{
   const auto runeCount = title.rune_count();
   std::vector<wchar_t> wCharTitle(runeCount + 1);
   std::transform(title.begin(), title.end(), wCharTitle.begin(), [](const Rune r) { return static_cast<wchar_t>(r); });
   wCharTitle[runeCount] = 0;

   const auto style = map_window_attributes_to_style(flags, isPopup);
   const auto styleEx = map_window_attributes_to_ex_style(flags, isPopup);

   RECT rect{
      /*left*/ 0,
      /*top*/ 0,
      /*right*/ dimension.x,
      /*botton*/ dimension.y,
   };
   if (!AdjustWindowRectEx(&rect, style, false, styleEx)) {
      spdlog::error("failed to calculate window size");
      return nullptr;
   }

   Vector2i clientDim{rect.right - rect.left, rect.bottom - rect.top};

   return CreateWindowExW(styleEx, g_windowClassName, wCharTitle.data(), style, map_window_attributes_to_x_position(flags, dimension),
                          map_window_attributes_to_y_position(flags, dimension), clientDim.x, clientDim.y, nullptr, nullptr, instance,
                          lpParam);
}

}// namespace

Surface::Surface(Display& display, const StringView title, const Vector2i dimension, const WindowAttributeFlags flags, const bool isPopup) :
    m_display(display),
    m_dimension(dimension),
    m_windowHandle(create_window(display.winapi_instance(), title, dimension, flags, this, isPopup))
{
   ShowWindow(m_windowHandle, SW_NORMAL);
   UpdateWindow(m_windowHandle);
}

Surface::~Surface()
{
   DestroyWindow(m_windowHandle);
}

void Surface::lock_cursor()
{
   RECT rect{};
   GetWindowRect(m_windowHandle, &rect);
   SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
   SetCapture(m_windowHandle);
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
   return GetCapture() == m_windowHandle;
}

Dimension Surface::dimension() const
{
   return m_dimension;
}

void Surface::set_keyboard_input_mode(const KeyboardInputModeFlags mode)
{
   m_keyboardInputMode = mode;
}

HINSTANCE Surface::winapi_instance() const
{
   return m_display.winapi_instance();
}

HWND Surface::winapi_window_handle() const
{
   return m_windowHandle;
}

void Surface::on_key_down(const WPARAM keyCode) const
{
   event_OnKeyIsPressed.publish(translate_key(keyCode));
}

void Surface::on_key_up(WPARAM keyCode) const
{
   event_OnKeyIsReleased.publish(translate_key(keyCode));
}

void Surface::on_button_down(MouseButton button) const
{
   if (m_currentCursor != nullptr) {
      SetCursor(m_currentCursor);
   }
   event_OnMouseButtonIsPressed.publish(button);
}

void Surface::on_button_up(MouseButton button) const
{
   event_OnMouseButtonIsReleased.publish(button);
}

void Surface::on_mouse_move(const int x, const int y)
{
   if (m_currentCursor != nullptr) {
      SetCursor(m_currentCursor);
   }

   if (this->is_cursor_locked()) {
      POINT point{};
      GetCursorPos(&point);

      RECT rect{};
      GetWindowRect(m_windowHandle, &rect);
      const auto originX = (rect.left + rect.right) / 2;
      const auto originY = (rect.top + rect.bottom) / 2;

      const int diffX = point.x - originX;
      const int diffY = point.y - originY;

      SetCursorPos(originX, originY);

      event_OnMouseRelativeMove.publish(Vector2{0.5f * static_cast<float>(diffX), 0.5f * static_cast<float>(diffY)});
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
   if (m_keyboardInputMode & KeyboardInputMode::Text) {
      event_OnTextInput.publish(rune);
   }
}

void Surface::on_set_focus(const bool hasFocus)
{
   if (hasFocus) {
      POINT point;
      if (GetCursorPos(&point) && ScreenToClient(m_windowHandle, &point)) {
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

LRESULT Surface::handle_window_event(const UINT msg, const WPARAM wParam, const LPARAM lParam)
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
      this->on_key_down(wParam);
      break;
   }
   case WM_KEYUP: {
      this->on_key_up(wParam);
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
      this->on_mouse_move(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      break;
   }
   case WM_SIZE: {
      this->on_resize(LOWORD(lParam), HIWORD(lParam));
      break;
   }
   case WM_CHAR: {
      this->on_rune(static_cast<Rune>(wParam));
      break;
   }
   case WM_INPUT: {
      spdlog::info("WM_INPUT event!");
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
      const auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
      this->on_mouse_wheel(delta);
      break;
   }
   default:
      return DefWindowProcW(m_windowHandle, msg, wParam, lParam);
   }

   return 0;
}

void Surface::set_cursor_icon(const CursorIcon icon)
{
   if (m_currentCursor != nullptr) {
      DestroyCursor(m_currentCursor);
   }

   switch (icon) {
   case CursorIcon::Arrow:
      m_currentCursor = LoadCursor(nullptr, IDC_ARROW);
      break;
   case CursorIcon::Hand:
      m_currentCursor = LoadCursor(nullptr, IDC_HAND);
      break;
   case CursorIcon::Move:
      m_currentCursor = LoadCursor(nullptr, IDC_SIZEALL);
      break;
   case CursorIcon::Wait:
      m_currentCursor = LoadCursor(nullptr, IDC_WAIT);
      break;
   case CursorIcon::Edit:
      m_currentCursor = LoadCursor(nullptr, IDC_IBEAM);
      break;
   case CursorIcon::ResizeHorizontal:
      m_currentCursor = LoadCursor(nullptr, IDC_SIZEWE);
      break;
   case CursorIcon::ResizeVertical:
      m_currentCursor = LoadCursor(nullptr, IDC_SIZENS);
      break;
   default:
      m_currentCursor = nullptr;
      return;
   }

   SetCursor(m_currentCursor);
   ShowCursor(true);
}

std::shared_ptr<ISurface> Surface::create_popup(const Vector2u dimensions, const Vector2 offset, const WindowAttributeFlags flags)
{
   auto popupSurface = std::make_shared<Surface>(m_display, "popup"_strv, dimensions, flags, true);

   RECT parentRect;
   GetClientRect(m_windowHandle, &parentRect);
   ClientToScreen(m_windowHandle, reinterpret_cast<LPPOINT>(&parentRect.left));

   SetWindowPos(popupSurface->m_windowHandle, HWND_TOP, parentRect.left + static_cast<int>(offset.x),
                parentRect.top + static_cast<int>(offset.y), 0, 0, SWP_NOSIZE);

   return popupSurface;
}

Display& Surface::display()
{
   return m_display;
}

}// namespace triglav::desktop
