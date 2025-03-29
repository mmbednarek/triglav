#pragma once

#include "Desktop.hpp"
#include "IDisplay.hpp"
#include "api/CursorShape.h"
#include "api/Decoration.h"
#include "api/PointerConstraints.h"
#include "api/RelativePointer.h"
#include "api/XdgShell.h"

#include <map>
#include <string_view>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

namespace triglav::desktop {

class Surface;

class Display final : public IDisplay
{
   friend Surface;

 public:
   Display();
   ~Display() override;

   void dispatch_messages() override;

   std::shared_ptr<ISurface> create_surface(std::string_view title, Vector2u dimensions, WindowAttributeFlags flags) override;

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
      return m_wmBase;
   }

   [[nodiscard]] constexpr wl_pointer* pointer() const noexcept
   {
      return m_pointer;
   }

   [[nodiscard]] constexpr zwp_pointer_constraints_v1* pointer_constraints() const noexcept
   {
      return m_pointerConstraints;
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

   wl_display* m_display;
   wl_registry* m_registry;
   wl_registry_listener m_registryListener{};
   wl_compositor* m_compositor{};
   xdg_wm_base* m_wmBase{};
   xdg_wm_base_listener m_wmBaseListener{};
   wl_seat* m_seat{};
   wl_seat_listener m_seatListener{};
   wl_pointer* m_pointer{};
   wl_pointer_listener m_pointerListener{};
   wl_keyboard* m_keyboard{};
   wl_keyboard_listener m_keyboardListener{};
   zwp_pointer_constraints_v1* m_pointerConstraints{};
   zwp_relative_pointer_manager_v1* m_relativePointerManager{};
   zwp_relative_pointer_v1* m_relativePointer{};
   zwp_relative_pointer_v1_listener m_relativePointerListener{};
   wp_cursor_shape_manager_v1* m_cursorShapeManager{};
   wp_cursor_shape_device_v1* m_cursorShapeDevice{};
   zxdg_decoration_manager_v1* m_decorationManager{};
   xkb_context* m_xkbContext;
   xkb_keymap* m_xkbKeymap;

   std::map<wl_surface*, Surface*> m_surfaceMap{};
   Surface* m_pointerSurface{};
   Surface* m_keyboardSurface{};
};

}// namespace triglav::desktop