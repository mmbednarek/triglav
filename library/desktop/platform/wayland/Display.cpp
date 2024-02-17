#include "Display.h"

#include "api/RelativePointer.h"
#include "Surface.h"

#include "VulkanSurface.h"

#include <cassert>
#include <format>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

namespace triglav::desktop {

Display::Display() :
    m_display(wl_display_connect(nullptr)),
    m_registry(wl_display_get_registry(m_display)),
    m_xkbContext(xkb_context_new(XKB_CONTEXT_NO_FLAGS))
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

   m_pointerListener.enter = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *surface,
                                wl_fixed_t surface_x, wl_fixed_t surface_y) {
      auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_enter(serial, surface, surface_x, surface_y);
   };

   m_pointerListener.leave = [](void *data, wl_pointer *wl_pointer, uint32_t serial, wl_surface *surface) {
      auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_leave(serial, surface);
   };

   m_pointerListener.motion = [](void *data, struct wl_pointer *wl_pointer, uint32_t time,
                                 wl_fixed_t surface_x, wl_fixed_t surface_y) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_motion(time, surface_x, surface_y);
   };

   m_pointerListener.button = [](void *data, wl_pointer *wl_pointer, uint32_t serial, uint32_t time,
                                 uint32_t button, uint32_t state) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_button(serial, time, button, state);
   };

   m_pointerListener.axis = [](void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis,
                               wl_fixed_t value) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_axis(time, axis, value);
   };

   m_pointerListener.frame = [](void *data, struct wl_pointer *wl_pointer) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointerListener.axis_source = [](void *data, wl_pointer *wl_pointer, uint32_t axis_source) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointerListener.axis_stop = [](void *data, wl_pointer *wl_pointer, uint32_t time, uint32_t axis) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointerListener.axis_discrete = [](void *data, wl_pointer *wl_pointer, uint32_t axis, int32_t discrete) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointerListener.axis_value120 = [](void *data, wl_pointer *wl_pointer, uint32_t axis, int32_t value120) {
      const auto *display = static_cast<Display *>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointerListener.axis_relative_direction = [](void *data, struct wl_pointer *wl_pointer, uint32_t axis,
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

   m_keyboardListener.keymap = [](void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd,
                                  uint32_t size) {
      auto *display = static_cast<Display *>(data);
      display->on_keymap(format, fd, size);
   };

   m_keyboardListener.enter = [](void *data, wl_keyboard *wl_keyboard, uint32_t serial, wl_surface *surface,
                                 wl_array *keys) {
      auto *display = static_cast<Display *>(data);
      display->on_keyboard_enter(serial, surface, keys);
   };

   m_keyboardListener.leave = [](void *data, wl_keyboard *wl_keyboard, uint32_t serial, wl_surface *surface) {
      const auto *display = static_cast<Display *>(data);
   };

   m_keyboardListener.key = [](void *data, wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time,
                               uint32_t key, uint32_t state) {
      const auto *display = static_cast<Display *>(data);
      display->on_key(serial, time, key, state);
   };

   m_keyboardListener.modifiers = [](void *data, wl_keyboard *wl_keyboard, uint32_t serial,
                                     uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
                                     uint32_t group) {
      const auto *display = static_cast<Display *>(data);
      std::cout << "modifiers state changes\n";
   };

   m_keyboardListener.repeat_info = [](void *data, wl_keyboard *wl_keyboard, int32_t rate, int32_t delay) {
      const auto *display = static_cast<Display *>(data);
   };


   wl_registry_add_listener(m_registry, &m_registryListener, this);
   wl_display_roundtrip(m_display);
}

Display::~Display()
{
   xkb_context_unref(m_xkbContext);
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
      wl_pointer_add_listener(m_pointer, &m_pointerListener, this);

      if (m_relativePointerManager) {
         m_relativePointer =
                 zwp_relative_pointer_manager_v1_get_relative_pointer(m_relativePointerManager, m_pointer);
         zwp_relative_pointer_v1_add_listener(m_relativePointer, &m_relativePointerListener, this);
      }
   }

   if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0) {
      m_keyboard = wl_seat_get_keyboard(m_seat);
      wl_keyboard_add_listener(m_keyboard, &m_keyboardListener, this);
   }
}

void Display::on_pointer_enter(const uint32_t serial, wl_surface *surface, const int32_t x, const int32_t y)
{
   if (not m_surfaceMap.contains(surface))
      return;

   m_pointerSurface = m_surfaceMap.at(surface);

   if (m_pointerSurface == nullptr)
      return;

   m_pointerSurface->m_pointerSerial = serial;
   m_pointerSurface->event_listener().on_mouse_enter(static_cast<float>(x) / 256.0f,
                                                     static_cast<float>(y) / 256.0f);
}

void Display::on_pointer_leave(uint32_t /*serial*/, wl_surface *surface)
{
   if (not m_surfaceMap.contains(surface))
      return;

   if (m_pointerSurface == nullptr)
      return;

   assert(m_pointerSurface == m_surfaceMap.at(surface));

   m_pointerSurface->event_listener().on_mouse_leave();

   m_pointerSurface = nullptr;
}

void Display::on_pointer_motion(uint32_t /*time*/, const int32_t x, const int32_t y) const
{
   if (m_pointerSurface == nullptr)
      return;

   m_pointerSurface->event_listener().on_mouse_move(static_cast<float>(x) / 256.0f,
                                                    static_cast<float>(y) / 256.0f);
}

void Display::dispatch_messages() const
{
   wl_display_dispatch(m_display);
}

std::unique_ptr<ISurface> Display::create_surface(int width, int height)
{
   return std::make_unique<Surface>(*this);
}

void Display::register_surface(wl_surface *wayland_surface, Surface *surface)
{
   m_surfaceMap.emplace(wayland_surface, surface);
}

void Display::on_pointer_relative_motion(uint32_t /*utime_hi*/, uint32_t /*utime_lo*/, const int32_t dx,
                                         const int32_t dy, int32_t /*dx_unaccel*/,
                                         int32_t /*dy_unaccel*/) const
{
   if (m_pointerSurface == nullptr)
      return;
   m_pointerSurface->event_listener().on_mouse_relative_move(static_cast<float>(dx) / 256.0f,
                                                             static_cast<float>(dy) / 256.0f);
}

void Display::on_pointer_axis(uint32_t time, uint32_t axis, int32_t value) const
{
   if (m_pointerSurface == nullptr)
      return;
   m_pointerSurface->event_listener().on_mouse_wheel_turn(static_cast<float>(value) / 2560.0f);
}

void Display::on_pointer_button(uint32_t serial, uint32_t time, const uint32_t button,
                                const uint32_t state) const
{
   if (m_pointerSurface == nullptr)
      return;

   if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
      m_pointerSurface->event_listener().on_mouse_button_is_pressed(button);
   } else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
      m_pointerSurface->event_listener().on_mouse_button_is_released(button);
   }
}

void Display::on_keyboard_enter(uint32_t serial, wl_surface *surface, wl_array *wls)
{
   if (not m_surfaceMap.contains(surface))
      return;

   m_keyboardSurface = m_surfaceMap.at(surface);
}

void Display::on_keymap(const uint32_t format, const int32_t fd, const uint32_t size)
{
   if (format != 1)
      return;

   auto *mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);

   m_xkbKeymap = xkb_keymap_new_from_string(m_xkbContext, static_cast<const char *>(mapped),
                                            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

   munmap(mapped, size);
   close(fd);
}

void Display::on_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) const
{
   if (m_keyboardSurface == nullptr)
      return;

   if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
      m_keyboardSurface->event_listener().on_key_is_pressed(key);
   } else if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
      m_keyboardSurface->event_listener().on_key_is_released(key);
   }
}

std::unique_ptr<IDisplay> get_display()
{
   return std::make_unique<Display>();
}

}// namespace wayland