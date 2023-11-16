#pragma once

#include "xdg-shell-client-protocol.h"

#include <wayland-client.h>
#include <string_view>
#include <functional>

namespace wayland {

using MouseMoveCallback = std::function<void(float pos_x, float pos_y)>;

class Display
{
 public:
   Display();
   ~Display();

   MouseMoveCallback OnMouseMove;

   void on_register_global(uint32_t name, std::string_view interface, uint32_t version);
   void on_xdg_ping(uint32_t serial) const;
   void on_seat_capabilities(uint32_t capabilities);
   void on_seat_name(std::string_view name) const;
   void on_pointer_motion(uint32_t time, int32_t x, int32_t y) const;
   void on_pointer_enter(uint32_t serial, wl_surface * surface, int32_t x, int32_t y) const;
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
   wl_seat *m_seat{};
   wl_seat_listener m_seatListener{};
   wl_pointer *m_pointer{};
   wl_pointer_listener m_poinerListener{};
};

}// namespace wayland