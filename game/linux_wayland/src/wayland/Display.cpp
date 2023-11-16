#include "Display.h"

#include <cassert>
#include <format>
#include <iostream>

namespace wayland {

Display::Display() :
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

   m_seatListener.name = [](void *data, wl_seat *seat, const char *name) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_seat == seat);
      display->on_seat_name(name);
   };

   m_poinerListener.enter = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *surface,
                               wl_fixed_t surface_x, wl_fixed_t surface_y) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_enter(serial, surface, surface_x, surface_y);
   };

   m_poinerListener.leave = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *surface) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
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
   };

   m_poinerListener.axis = [](void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis,
                              wl_fixed_t value) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
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
}

void Display::on_xdg_ping(const uint32_t serial) const
{
   xdg_wm_base_pong(m_wmBase, serial);
}

void Display::on_seat_capabilities(uint32_t capabilities)
{
   if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0) {
      m_pointer = wl_seat_get_pointer(m_seat);
      wl_pointer_add_listener(m_pointer, &m_poinerListener, this);
   }
}

void Display::on_seat_name(std::string_view name) const
{
   std::cout << std::format("seat name: {}\n", name);
}

void Display::on_pointer_enter(uint32_t serial, wl_surface *surface, int32_t x, int32_t y) const
{
   std::cout << std::format("pointer enter: {} {}\n", x, y);
}

void Display::on_pointer_motion(uint32_t time, int32_t x, int32_t y) const
{
   if (OnMouseMove) {
      OnMouseMove(static_cast<float>(x) / 256.0f, static_cast<float>(y) / 256.0f);
   }
}

void Display::dispatch() const
{
   wl_display_dispatch(m_display);
}

}// namespace wayland