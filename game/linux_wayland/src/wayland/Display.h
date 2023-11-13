#pragma once

#include "xdg-shell-client-protocol.h"

#include <wayland-client.h>
#include <string_view>

namespace wayland {

class Display
{
 public:
   Display();
   ~Display();

   void on_register_global(uint32_t name, std::string_view interface, uint32_t version);
   void on_xdg_ping(uint32_t serial) const;
   void dispatch() const;

   [[nodiscard]] constexpr wl_display *display() const noexcept
   {
      return m_display;
   }
   [[nodiscard]] constexpr wl_compositor *compositor() const noexcept
   {
      return m_compositor;
   }

   [[nodiscard]] constexpr xdg_wm_base *wm_base() const noexcept
   {
      return m_wmBase;
   }
 private:
   wl_display *m_display;
   wl_registry *m_registry;
   wl_registry_listener m_registryListener{};
   wl_compositor *m_compositor{};
   xdg_wm_base *m_wmBase{};
   xdg_wm_base_listener m_wmBaseListener{};
};

}// namespace wayland