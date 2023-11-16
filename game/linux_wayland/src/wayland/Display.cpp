#include "Display.h"


#include "api/RelativePointer.h"

#include <cassert>
#include <format>
#include <iostream>

namespace wayland {

Display::Display(IDisplayEventListener &eventListener) :
    m_eventListener(eventListener),
    m_display(wl_display_connect(nullptr)),
    m_registry(wl_display_get_registry(m_display))
{
   m_registryListener.global = [](void *data, wl_registry *wl_registry, const uint32_t name,
                                  const char *interface, const uint32_t version) {
      auto *display = static_cast<Display *>(data);
      assert(display->m_registry == wl_registry);
      display->on_register_global(name, interface, version);
   };

   m_registryListener.global_remove = [](void * /*data*/, struct wl_registry * /*wl_registry*/,
                                         uint32_t /*name*/) {};

   m_wmBaseListener.ping = [](void *data, xdg_wm_base *xdg_wm_base, const uint32_t serial) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_wmBase == xdg_wm_base);
      display->on_xdg_ping(serial);
   };

   m_seatListener.capabilities = [](void *data, wl_seat *seat, const uint32_t capabilities) {
      auto *display = static_cast<Display *>(data);
      assert(display->m_seat == seat);
      display->on_seat_capabilities(capabilities);
   };

   m_seatListener.name = [](void *, wl_seat *, const char *) {};

   m_poinerListener.enter = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *surface,
                               wl_fixed_t surface_x, wl_fixed_t surface_y) {
      auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_enter(serial, surface, surface_x, surface_y);
   };

   m_poinerListener.leave = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *surface) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_leave(serial, surface);
   };

   m_poinerListener.motion = [](void *data, struct wl_pointer *wl_pointer, uint32_t time,
                                wl_fixed_t surface_x, wl_fixed_t surface_y) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_motion(time, surface_x, surface_y);
   };

   m_poinerListener.button = [](void *data, wl_pointer *wl_pointer, uint32_t serial, uint32_t time,
                                uint32_t button, uint32_t state) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_button(serial, time, button, state);
   };

   m_poinerListener.axis = [](void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis,
                              wl_fixed_t value) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_axis(time, axis, value);
   };

   m_poinerListener.frame = [](void *data, struct wl_pointer *wl_pointer) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_poinerListener.axis_source = [](void *data, wl_pointer *wl_pointer, uint32_t axis_source) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_poinerListener.axis_stop = [](void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_poinerListener.axis_discrete = [](void *data, wl_pointer *wl_pointer, uint32_t axis, int32_t discrete) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_poinerListener.axis_value120 = [](void *data, wl_pointer *wl_pointer, uint32_t axis, int32_t value120) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_poinerListener.axis_relative_direction = [](void *data, struct wl_pointer *wl_pointer, uint32_t axis,
                                                 uint32_t direction) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_relativePointerListener.relative_motion =
           [](void *data, zwp_relative_pointer_v1 *zwp_relative_pointer_v1, uint32_t utime_hi,
              uint32_t utime_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel) {
              const auto *display = static_cast<Display *>(data);
              assert(display->m_relativePointer == zwp_relative_pointer_v1);
              display->on_pointer_relative_motion(utime_hi, utime_lo, dx, dy, dx_unaccel, dy_unaccel);
           };

   wl_registry_add_listener(m_registry, &m_registryListener, this);
   wl_display_roundtrip(m_display);
}

Display::~Display()
{
   xdg_wm_base_destroy(m_wmBase);
   wl_compositor_destroy(m_compositor);
   wl_registry_destroy(m_registry);
   wl_display_disconnect(m_display);
}

void Display::on_register_global(const uint32_t name, const std::string_view interface, uint32_t /*version*/)
{
   std::cout << interface << '\n';

   if (interface == wl_compositor_interface.name) {
      m_compositor =
              static_cast<wl_compositor *>(wl_registry_bind(m_registry, name, &wl_compositor_interface, 4));
      return;
   }
   if (interface == xdg_wm_base_interface.name) {
      m_wmBase = static_cast<xdg_wm_base *>(wl_registry_bind(m_registry, name, &xdg_wm_base_interface, 1));
      xdg_wm_base_add_listener(m_wmBase, &m_wmBaseListener, this);
   }
   if (interface == wl_seat_interface.name) {
      m_seat = static_cast<wl_seat *>(wl_registry_bind(m_registry, name, &wl_seat_interface, 7));
      wl_seat_add_listener(m_seat, &m_seatListener, this);
   }
   if (interface == zwp_pointer_constraints_v1_interface.name) {
      m_pointerConstraints = static_cast<zwp_pointer_constraints_v1 *>(
              wl_registry_bind(m_registry, name, &zwp_pointer_constraints_v1_interface, 1));
   }
   if (interface == zwp_relative_pointer_manager_v1_interface.name) {
      m_relativePointerManager = static_cast<zwp_relative_pointer_manager_v1 *>(
              wl_registry_bind(m_registry, name, &zwp_relative_pointer_manager_v1_interface, 1));
   }
}

void Display::on_xdg_ping(const uint32_t serial) const
{
   xdg_wm_base_pong(m_wmBase, serial);
}

void Display::on_seat_capabilities(const uint32_t capabilities)
{
   if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0) {
      m_pointer = wl_seat_get_pointer(m_seat);
      wl_pointer_add_listener(m_pointer, &m_poinerListener, this);

      if (m_relativePointerManager) {
         m_relativePointer =
                 zwp_relative_pointer_manager_v1_get_relative_pointer(m_relativePointerManager, m_pointer);
         zwp_relative_pointer_v1_add_listener(m_relativePointer, &m_relativePointerListener, this);
      }
   }
}

void Display::on_pointer_enter(const uint32_t serial, wl_surface *surface, const int32_t x,
                               const int32_t y)
{
   if (not m_surfaceMap.contains(surface))
      return;

   m_pointerEnterSerial = serial;
   m_eventListener.on_pointer_enter_surface(*m_surfaceMap.at(surface), static_cast<float>(x) / 256.0f,
                                            static_cast<float>(y) / 256.0f);
}

void Display::on_pointer_leave(uint32_t /*serial*/, wl_surface *surface) const
{
   if (not m_surfaceMap.contains(surface))
      return;

   m_eventListener.on_pointer_leave_surface(*m_surfaceMap.at(surface));
}

void Display::on_pointer_motion(uint32_t /*time*/, const int32_t x, const int32_t y) const
{
   m_eventListener.on_pointer_change_position(static_cast<float>(x) / 256.0f, static_cast<float>(y) / 256.0f);
}

void Display::dispatch() const
{
   wl_display_dispatch(m_display);
}

void Display::register_surface(wl_surface *wayland_surface, Surface *surface)
{
   m_surfaceMap.emplace(wayland_surface, surface);
}

void Display::on_pointer_relative_motion(uint32_t /*utime_hi*/, uint32_t /*utime_lo*/, const int32_t dx,
                                         const int32_t dy, int32_t /*dx_unaccel*/,
                                         int32_t /*dy_unaccel*/) const
{
   m_eventListener.on_pointer_relative_motion(static_cast<float>(dx) / 256.0f,
                                              static_cast<float>(dy) / 256.0f);
}

void Display::on_pointer_axis(uint32_t time, uint32_t axis, int32_t value) const
{
   m_eventListener.on_mouse_wheel_turn(static_cast<float>(value) / 2560.0f);
}

void Display::on_pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state) const
{
   if (state == 0) {
      m_eventListener.on_mouse_button_is_pressed(button);
   }
}

void Display::hide_pointer() const
{
   wl_pointer_set_cursor(m_pointer, m_pointerEnterSerial, nullptr, 0, 0);
}

}// namespace wayland