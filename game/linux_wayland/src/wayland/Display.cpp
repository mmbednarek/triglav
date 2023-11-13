#include "Display.h"


#include <cassert>

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
      static_cast<Display *>(data)->on_xdg_ping(serial);
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
}

void Display::on_xdg_ping(const uint32_t serial) const
{
   xdg_wm_base_pong(m_wmBase, serial);
}

void Display::dispatch() const
{
   wl_display_dispatch(m_display);
}

}// namespace wayland