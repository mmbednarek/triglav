#include "Surface.h"
#include "ISurfaceEventListener.hpp"

#include <iostream>

namespace triglav::desktop::x11 {

namespace {

Key map_key(const KeyCode keyCode)
{
   switch (keyCode) {
   case 25: return Key::W;
   case 39: return Key::S;
   case 38: return Key::A;
   case 40: return Key::D;
   case 24: return Key::Q;
   case 26: return Key::E;
   case 65: return Key::Space;
   case 67: return Key::F1;
   case 68: return Key::F2;
   case 69: return Key::F3;
   case 70: return Key::F4;
   case 71: return Key::F5;
   case 72: return Key::F6;
   case 73: return Key::F7;
   }

   return Key::Unknown;
}

MouseButton map_button(const uint32_t keyCode)
{
   switch (keyCode) {
   case 1: return MouseButton::Left;
   case 2: return MouseButton::Middle;
   case 3: return MouseButton::Right;
   }
   return MouseButton::Unknown;
}

}// namespace

Surface::Surface(::Display *display, const Window window, const Dimension dimension) :
    m_display(display),
    m_window(window),
    m_dimension(dimension)
{
}

void Surface::lock_cursor()
{
   m_isCursorLocked = true;
   std::cout << "locking!\n";

   XGrabPointer(m_display, m_window, false,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask | EnterWindowMask |
                        LeaveWindowMask,
                GrabModeAsync, GrabModeAsync, RootWindow(m_display, DefaultScreen(m_display)), None,
                CurrentTime);
   Dimension center{m_dimension.width / 2, m_dimension.height / 2};
   XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, center.width, center.height);
   XSync(m_display, false);
}

void Surface::unlock_cursor()
{
   std::cout << "unlocking!\n";
   m_isCursorLocked = false;
   XUngrabPointer(m_display, CurrentTime);
}

void Surface::hide_cursor() const
{
   // TODO: Implement
}

void Surface::add_event_listener(ISurfaceEventListener *eventListener)
{
   m_listeners.emplace_back(eventListener);
}

bool Surface::is_cursor_locked() const
{
   return m_isCursorLocked;
}

Dimension Surface::dimension() const
{
   return m_dimension;
}

void Surface::dispatch_key_press(const KeyCode code) const
{
   std::cout << "PRESS: " << static_cast<int>(code) << '\n';
   for (ISurfaceEventListener *listener : m_listeners) {
      listener->on_key_is_pressed(map_key(code));
   }
}

void Surface::dispatch_key_release(const KeyCode code) const
{
   std::cout << "RELEASE: " << static_cast<int>(code) << '\n';
   for (ISurfaceEventListener *listener : m_listeners) {
      listener->on_key_is_released(map_key(code));
   }
}

void Surface::dispatch_button_press(const uint32_t code) const
{
   std::cout << "MOUSE PRESS: " << static_cast<int>(code) << '\n';
   for (ISurfaceEventListener *listener : m_listeners) {
      listener->on_mouse_button_is_pressed(map_button(code));
   }
}

void Surface::dispatch_button_release(const uint32_t code) const
{
   std::cout << "MOUSE RELEASE: " << static_cast<int>(code) << '\n';
   for (ISurfaceEventListener *listener : m_listeners) {
      listener->on_mouse_button_is_released(map_button(code));
   }
}

void Surface::dispatch_mouse_move(const int x, const int y) const
{
   if (m_isCursorLocked) {
      Dimension center{m_dimension.width / 2, m_dimension.height / 2};
      const auto dx = x - center.width;
      const auto dy = y - center.height;

      for (ISurfaceEventListener *listener : m_listeners) {
         listener->on_mouse_relative_move(static_cast<float>(dx), static_cast<float>(dy));
      }
   }
}

void Surface::tick() const
{
   if (m_isCursorLocked) {
      Dimension center{m_dimension.width / 2, m_dimension.height / 2};
      XWarpPointer(m_display, None, m_window, 0, 0, 0, 0, center.width, center.height);
      XSync(m_display, false);
   }
}

}// namespace triglav::desktop::x11
