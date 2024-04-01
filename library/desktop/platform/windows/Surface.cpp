#include "Surface.h"

#include <spdlog/spdlog.h>

namespace triglav::desktop {

namespace {

Key translate_key(WPARAM keyCode)
{
   switch (keyCode) {
   case 'W': return Key::W;
   case 'A': return Key::A;
   case 'S': return Key::S;
   case 'D': return Key::D;
   case VK_SPACE: return Key::Space;
   case VK_F1: return Key::F1;
   case VK_F2: return Key::F2;
   case VK_F3: return Key::F3;
   case VK_F4: return Key::F4;
   case VK_F5: return Key::F5;
   case VK_F6: return Key::F6;
   }

   return Key::Unknown;
}

}// namespace

Surface::Surface(const HINSTANCE instance, const Dimension dimension) :
    m_instance(instance),
    m_dimension(dimension),
    m_windowHandle(CreateWindowExA(WS_EX_CLIENTEDGE, g_windowClassName, "Triglav Engine", WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT, CW_USEDEFAULT, m_dimension.width, m_dimension.height,
                                   nullptr, nullptr, m_instance, this))
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

void Surface::add_event_listener(ISurfaceEventListener *eventListener)
{
   m_eventListener = eventListener;
}

bool Surface::is_cursor_locked() const
{
   return GetCapture() == m_windowHandle;
}

Dimension Surface::dimension() const
{
   return m_dimension;
}

HINSTANCE Surface::winapi_instance() const
{
   return m_instance;
}

HWND Surface::winapi_window_handle() const
{
   return m_windowHandle;
}

void Surface::on_key_down(WPARAM keyCode) const
{
   if (m_eventListener != nullptr) {
      m_eventListener->on_key_is_pressed(translate_key(keyCode));
   }
}

void Surface::on_key_up(WPARAM keyCode) const
{
   if (m_eventListener != nullptr) {
      m_eventListener->on_key_is_released(translate_key(keyCode));
   }
}

void Surface::on_button_down(MouseButton button) const
{
   if (m_eventListener != nullptr) {
      m_eventListener->on_mouse_button_is_pressed(button);
   }
}

void Surface::on_button_up(MouseButton button) const
{
   if (m_eventListener != nullptr) {
      m_eventListener->on_mouse_button_is_released(button);
   }
}

void Surface::on_mouse_move(const int x, const int y) const
{
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

      if (m_eventListener != nullptr) {
         m_eventListener->on_mouse_relative_move(0.5f * static_cast<float>(diffX),
                                                 0.5f * static_cast<float>(diffY));
      }
   } else {
      if (m_eventListener != nullptr) {
         m_eventListener->on_mouse_move(static_cast<float>(x), static_cast<float>(y));
      }
   }
}

void Surface::on_close() const
{
   if (m_eventListener != nullptr) {
      m_eventListener->on_close();
   }
}

}// namespace triglav::desktop
