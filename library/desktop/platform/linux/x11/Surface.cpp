#include "Surface.hpp"

#include "Desktop.hpp"
#include "Display.hpp"

#include <X11/cursorfont.h>
#include <spdlog/spdlog.h>

namespace triglav::desktop::x11 {

namespace {

Key map_key(const KeyCode keyCode)
{
   switch (keyCode) {
   case 22:
      return Key::Backspace;
   case 25:
      return Key::W;
   case 39:
      return Key::S;
   case 38:
      return Key::A;
   case 40:
      return Key::D;
   case 24:
      return Key::Q;
   case 26:
      return Key::E;
   case 65:
      return Key::Space;
   case 67:
      return Key::F1;
   case 68:
      return Key::F2;
   case 69:
      return Key::F3;
   case 70:
      return Key::F4;
   case 71:
      return Key::F5;
   case 72:
      return Key::F6;
   case 73:
      return Key::F7;
   case 74:
      return Key::F8;
   case 75:
      return Key::F9;
   case 76:
      return Key::F10;
   case 77:
      return Key::F11;
   case 78:
      return Key::F12;
   case 111:
      return Key::UpArrow;
   case 113:
      return Key::LeftArrow;
   case 114:
      return Key::RightArrow;
   case 116:
      return Key::DownArrow;
   }

   spdlog::warn("unknown keycode: {}", keyCode);
   return Key::Unknown;
}

MouseButton map_button(const uint32_t keyCode)
{
   switch (keyCode) {
   case 1:
      return MouseButton::Left;
   case 2:
      return MouseButton::Middle;
   case 3:
      return MouseButton::Right;
   }
   return MouseButton::Unknown;
}

}// namespace

Surface::Surface(Display& display, const Window window, const Dimension dimension) :
    m_display(display),
    m_window(window),
    m_dimension(dimension),
    m_xim(XOpenIM(m_display.x11_display(), nullptr, nullptr, nullptr))
{
}

Surface::~Surface()
{
   if (m_window != ~0u) {
      this->internal_close();
   }
}

void Surface::lock_cursor()
{
   m_isCursorLocked = true;

   XGrabPointer(m_display.x11_display(), m_window, false,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask,
                GrabModeAsync, GrabModeAsync, RootWindow(m_display.x11_display(), DefaultScreen(m_display.x11_display())), 0L, CurrentTime);
   Dimension center{m_dimension.x / 2, m_dimension.y / 2};
   XWarpPointer(m_display.x11_display(), 0L, m_window, 0, 0, 0, 0, center.x, center.y);
   XSync(m_display.x11_display(), false);
}

void Surface::unlock_cursor()
{
   m_isCursorLocked = false;
   XUngrabPointer(m_display.x11_display(), CurrentTime);
}

void Surface::hide_cursor() const
{
   // TODO: Implement
}

void Surface::internal_close()
{
   XDestroyWindow(m_display.x11_display(), m_window);
   m_window = ~0u;
}

bool Surface::is_cursor_locked() const
{
   return m_isCursorLocked;
}

Dimension Surface::dimension() const
{
   XWindowAttributes attribs;
   ::XGetWindowAttributes(m_display.x11_display(), m_window, &attribs);
   return {attribs.width, attribs.height};
}

void Surface::set_cursor_icon(const CursorIcon icon)
{
   spdlog::info("calling set_cursor_icon: {}", static_cast<int>(icon));

   if (m_currentCursor != 0) {
      XFreeCursor(m_display.x11_display(), m_currentCursor);
   }

   switch (icon) {
   case CursorIcon::Arrow:
      m_currentCursor = ::XCreateFontCursor(m_display.x11_display(), XC_arrow);
      break;
   case CursorIcon::Hand:
      m_currentCursor = ::XCreateFontCursor(m_display.x11_display(), XC_hand1);
      break;
   case CursorIcon::Move:
      m_currentCursor = ::XCreateFontCursor(m_display.x11_display(), XC_fleur);
      break;
   case CursorIcon::Wait:
      m_currentCursor = ::XCreateFontCursor(m_display.x11_display(), XC_watch);
      break;
   case CursorIcon::Edit:
      m_currentCursor = ::XCreateFontCursor(m_display.x11_display(), XC_xterm);
      break;
   case CursorIcon::ResizeHorizontal:
      m_currentCursor = ::XCreateFontCursor(m_display.x11_display(), XC_sb_h_double_arrow);
      break;
   case CursorIcon::ResizeVertical:
      m_currentCursor = ::XCreateFontCursor(m_display.x11_display(), XC_double_arrow);
      break;
   default:
      m_currentCursor = 0;
      return;
   }

   ::XDefineCursor(m_display.x11_display(), m_window, m_currentCursor);
}

void Surface::set_keyboard_input_mode(const KeyboardInputModeFlags mode)
{
   m_keyboardInputMode = mode;

   if (mode & KeyboardInputMode::Text) {
      ::XSetLocaleModifiers("");

      if (m_xic != nullptr) {
         ::XDestroyIC(m_xic);
      }

      m_xic =
         ::XCreateIC(m_xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, m_window, XNFocusWindow, m_window, nullptr);
      ::XSetICFocus(m_xic);
   }
}

std::shared_ptr<ISurface> Surface::create_popup(const Vector2u dimensions, const Vector2 offset, const WindowAttributeFlags /*flags*/)
{
   const auto popupWindow = XCreateSimpleWindow(m_display.x11_display(), m_window, static_cast<int>(offset.x), static_cast<int>(offset.y),
                                                dimensions.x, dimensions.y, 0, 0, 0xffffffff);

   XMapWindow(m_display.x11_display(), popupWindow);
   XSelectInput(m_display.x11_display(), popupWindow,
                ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterNotify |
                   LeaveNotify);

   auto surface = std::make_shared<Surface>(m_display, popupWindow, dimensions);
   m_display.map_surface(popupWindow, surface);
   return surface;
}

void Surface::dispatch_key_press(const XEvent& event) const
{
   if (m_keyboardInputMode & KeyboardInputMode::Text) {
      char buffer[128];
      const int count = Xutf8LookupString(m_xic, const_cast<XKeyPressedEvent*>(&event.xkey), buffer, 128, nullptr, nullptr);
      const char* bufferPtr = buffer;
      auto rune = decode_rune_from_buffer(bufferPtr, buffer + count);
      event_OnTextInput.publish(rune);
   }
   if (m_keyboardInputMode & KeyboardInputMode::Direct) {
      event_OnKeyIsPressed.publish(map_key(event.xkey.keycode));
   }
}

void Surface::dispatch_key_release(const XEvent& event) const
{
   if (m_keyboardInputMode & KeyboardInputMode::Direct) {
      event_OnKeyIsReleased.publish(map_key(event.xkey.keycode));
   }
}

void Surface::dispatch_button_press(const XEvent& event) const
{
   if (event.xbutton.button == 4) {
      event_OnMouseWheelTurn.publish(-1.0);
   } else if (event.xbutton.button == 5) {
      event_OnMouseWheelTurn.publish(1.0);
   } else {
      event_OnMouseButtonIsPressed.publish(map_button(event.xbutton.button));
   }
}

void Surface::dispatch_button_release(const XEvent& event) const
{
   event_OnMouseButtonIsReleased.publish(map_button(event.xbutton.button));
}

void Surface::dispatch_mouse_move(const XEvent& event) const
{
   event_OnMouseMove.publish(Vector2{static_cast<float>(event.xmotion.x), static_cast<float>(event.xmotion.y)});
}

void Surface::dispatch_mouse_relative_move(const Vector2 diff) const
{
   if (not m_isCursorLocked)
      return;

   event_OnMouseRelativeMove.publish(diff);
}

void Surface::dispatch_close() const
{
   event_OnClose.publish();
}

void Surface::tick() const
{
   if (m_isCursorLocked) {
      const Dimension center{m_dimension.x / 2, m_dimension.y / 2};
      XWarpPointer(m_display.x11_display(), 0L, m_window, 0, 0, 0, 0, center.x, center.y);
      XSync(m_display.x11_display(), false);
   }
}

Display& Surface::display()
{
   return m_display;
}

const Display& Surface::display() const
{
   return m_display;
}

}// namespace triglav::desktop::x11
