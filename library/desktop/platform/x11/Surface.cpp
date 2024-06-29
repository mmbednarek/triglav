#include "Surface.h"
#include "ISurfaceEventListener.hpp"

#include <iostream>

namespace triglav::desktop::x11 {

namespace {

Key map_key(const KeyCode keyCode)
{
   switch (keyCode) {
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
   }

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

Surface::Surface(::Display* display, const Window window, const Dimension dimension) :
    m_display(display),
    m_window(window),
    m_dimension(dimension)
{
}

Surface::~Surface()
{
   if (m_display != nullptr) {
      this->internal_close();
   }
}

void Surface::lock_cursor()
{
   m_isCursorLocked = true;

   XGrabPointer(m_display, m_window, false,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask,
                GrabModeAsync, GrabModeAsync, RootWindow(m_display, DefaultScreen(m_display)), 0L, CurrentTime);
   Dimension center{m_dimension.width / 2, m_dimension.height / 2};
   XWarpPointer(m_display, 0L, m_window, 0, 0, 0, 0, center.width, center.height);
   XSync(m_display, false);
}

void Surface::unlock_cursor()
{
   m_isCursorLocked = false;
   XUngrabPointer(m_display, CurrentTime);
}

void Surface::hide_cursor() const
{
   // TODO: Implement
}

void Surface::add_event_listener(ISurfaceEventListener* eventListener)
{
   m_listeners.emplace_back(eventListener);
}

void Surface::internal_close()
{
   XDestroyWindow(m_display, m_window);
   m_window = ~0;
   m_display = nullptr;
}

bool Surface::is_cursor_locked() const
{
   return m_isCursorLocked;
}

Dimension Surface::dimension() const
{
   XWindowAttributes attribs;
   ::XGetWindowAttributes(m_display, m_window, &attribs);
   return {attribs.width, attribs.height};
}

void Surface::dispatch_key_press(const KeyCode code) const
{
   for (ISurfaceEventListener* listener : m_listeners) {
      listener->on_key_is_pressed(map_key(code));
   }
}

void Surface::dispatch_key_release(const KeyCode code) const
{
   for (ISurfaceEventListener* listener : m_listeners) {
      listener->on_key_is_released(map_key(code));
   }
}

void Surface::dispatch_button_press(const uint32_t code) const
{
   for (ISurfaceEventListener* listener : m_listeners) {
      listener->on_mouse_button_is_pressed(map_button(code));
   }
}

void Surface::dispatch_button_release(const uint32_t code) const
{
   for (ISurfaceEventListener* listener : m_listeners) {
      listener->on_mouse_button_is_released(map_button(code));
   }
}

void Surface::dispatch_mouse_move(const int x, const int y) const
{
   for (ISurfaceEventListener* listener : m_listeners) {
      listener->on_mouse_move(static_cast<float>(x), static_cast<float>(y));
   }
}

void Surface::dispatch_mouse_relative_move(float x, float y) const
{
   if (not m_isCursorLocked)
      return;

   for (ISurfaceEventListener* listener : m_listeners) {
      listener->on_mouse_relative_move(x, y);
   }
}

void Surface::tick() const
{
   if (m_isCursorLocked) {
      Dimension center{m_dimension.width / 2, m_dimension.height / 2};
      XWarpPointer(m_display, 0L, m_window, 0, 0, 0, 0, center.width, center.height);
      XSync(m_display, false);
   }
}

}// namespace triglav::desktop::x11
