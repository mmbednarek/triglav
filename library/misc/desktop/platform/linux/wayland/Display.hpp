#pragma once

#include "Desktop.hpp"
#include "IDisplay.hpp"
extern "C"
{
#include "api/CursorShape.h"
#include "api/Decoration.h"
#include "api/PointerConstraints.h"
#include "api/RelativePointer.h"
#include "api/XdgShell.h"
}

#include "triglav/String.hpp"
#include "triglav/threading/SafeAccess.hpp"

#include <map>
#include <string_view>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

namespace triglav::desktop::wayland {

class Surface;

class Display final : public IDisplay
{
   friend Surface;

 public:
   Display();
   ~Display() override;

   void dispatch_messages() override;

   std::shared_ptr<ISurface> create_surface(StringView title, Vector2u dimensions, WindowAttributeFlags flags) override;

   [[nodiscard]] constexpr wl_display* display() const noexcept
   {
      return m_display;
   }

   [[nodiscard]] constexpr wl_compositor* compositor() const noexcept
   {
      return m_compositor;
   }

   [[nodiscard]] constexpr xdg_wm_base* wm_base() const noexcept
   {
      return m_wm_base;
   }

   [[nodiscard]] constexpr wl_pointer* pointer() const noexcept
   {
      return m_pointer;
   }

   [[nodiscard]] constexpr zwp_pointer_constraints_v1* pointer_constraints() const noexcept
   {
      return m_pointer_constraints;
   }

 private:
   void register_surface(wl_surface* wayland_surface, Surface* surface);

   void on_register_global(uint32_t name, std::string_view interface, uint32_t version);
   void on_xdg_ping(uint32_t serial) const;
   void on_seat_capabilities(uint32_t capabilities);
   void on_pointer_motion(uint32_t time, int32_t x, int32_t y) const;
   void on_pointer_enter(uint32_t serial, wl_surface* surface, int32_t x, int32_t y);
   void on_pointer_leave(uint32_t serial, wl_surface* surface);
   void on_pointer_relative_motion(uint32_t utime_hi, uint32_t utime_lo, int32_t dx, int32_t dy, int32_t dx_unaccel,
                                   int32_t dy_unaccel) const;
   void on_pointer_axis(uint32_t time, uint32_t axis, int32_t value) const;
   void on_pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state) const;
   void on_keyboard_enter(uint32_t serial, wl_surface* surface, wl_array* wls);
   void on_keymap(uint32_t format, int32_t fd, uint32_t size);
   void on_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) const;
   void on_destroyed_surface(Surface* surface);
   void on_modifiers(u32 serial, u32 mods_depressed, u32 mods_latched, u32 mods_locked, u32 group) const;

   wl_display* m_display;
   wl_registry* m_registry;
   wl_registry_listener m_registry_listener{};
   wl_compositor* m_compositor{};
   xdg_wm_base* m_wm_base{};
   xdg_wm_base_listener m_wm_base_listener{};
   wl_seat* m_seat{};
   wl_seat_listener m_seat_listener{};
   wl_pointer* m_pointer{};
   wl_pointer_listener m_pointer_listener{};
   wl_keyboard* m_keyboard{};
   wl_keyboard_listener m_keyboard_listener{};
   zwp_pointer_constraints_v1* m_pointer_constraints{};
   zwp_relative_pointer_manager_v1* m_relative_pointer_manager{};
   zwp_relative_pointer_v1* m_relative_pointer{};
   zwp_relative_pointer_v1_listener m_relative_pointer_listener{};
   wp_cursor_shape_manager_v1* m_cursor_shape_manager{};
   wp_cursor_shape_device_v1* m_cursor_shape_device{};
   zxdg_decoration_manager_v1* m_decoration_manager{};
   xkb_context* m_xkb_context;
   xkb_keymap* m_xkb_keymap;
   xkb_state* m_xkb_state;

   std::map<wl_surface*, Surface*> m_surface_map{};
   threading::SafeReadWriteAccess<Surface*> m_pointer_surface{};
   threading::SafeReadWriteAccess<Surface*> m_keyboard_surface{};
};

}// namespace triglav::desktop::wayland