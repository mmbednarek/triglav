#include "Display.hpp"

#include "Surface.hpp"
extern "C"
{
#include "api/CursorShape.h"
#include "api/Decoration.h"
#include "api/RelativePointer.h"
}

#include <cassert>
#include <iostream>
#include <ranges>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

namespace triglav::desktop::wayland {

namespace {

Key map_key(const u32 key)
{
   switch (key) {
   case 1:
      return Key::Escape;
   case 15:
      return Key::Tab;
   case 14:
      return Key::Backspace;
   case 16:
      return Key::Q;
   case 17:
      return Key::W;
   case 18:
      return Key::E;
   case 21:
      return Key::Y;
   case 28:
      return Key::Enter;
   case 29:
      return Key::Control;
   case 31:
      return Key::S;
   case 30:
      return Key::A;
   case 32:
      return Key::D;
   case 42:
      return Key::Shift;
   case 44:
      return Key::Z;
   case 56:
      return Key::Alt;
   case 57:
      return Key::Space;
   case 59:
      return Key::F1;
   case 60:
      return Key::F2;
   case 61:
      return Key::F3;
   case 62:
      return Key::F4;
   case 63:
      return Key::F5;
   case 64:
      return Key::F6;
   case 65:
      return Key::F7;
   case 66:
      return Key::F8;
   case 67:
      return Key::F9;
   case 68:
      return Key::F10;
   case 69:
      return Key::F11;
   case 70:
      return Key::F12;
   case 103:
      return Key::DownArrow;
   case 105:
      return Key::LeftArrow;
   case 106:
      return Key::RightArrow;
   case 108:
      return Key::UpArrow;
   }

   return Key::Unknown;
}

MouseButton map_button(const u32 button)
{
   switch (button) {
   case 272:
      return MouseButton::Left;
   case 273:
      return MouseButton::Right;
   case 274:
      return MouseButton::Middle;
   }

   return MouseButton::Unknown;
}

}// namespace

Display::Display() :
    m_display(wl_display_connect(nullptr)),
    m_registry(wl_display_get_registry(m_display)),
    m_xkb_context(xkb_context_new(XKB_CONTEXT_NO_FLAGS))
{
   m_registry_listener.global = [](void* data, [[maybe_unused]] wl_registry* wl_registry, const u32 name, const char* interface,
                                   const u32 version) {
      auto* display = static_cast<Display*>(data);
      assert(display->m_registry == wl_registry);
      display->on_register_global(name, interface, version);
   };

   m_registry_listener.global_remove = [](void* /*data*/, struct wl_registry* /*wl_registry*/, u32 /*name*/) {};

   m_wm_base_listener.ping = [](void* data, [[maybe_unused]] xdg_wm_base* xdg_wm_base, const u32 serial) {
      const auto* display = static_cast<Display*>(data);
      assert(display->m_wm_base == xdg_wm_base);
      display->on_xdg_ping(serial);
   };

   m_seat_listener.capabilities = [](void* data, [[maybe_unused]] wl_seat* seat, const u32 capabilities) {
      auto* display = static_cast<Display*>(data);
      assert(display->m_seat == seat);
      display->on_seat_capabilities(capabilities);
   };

   m_seat_listener.name = [](void*, wl_seat*, const char*) {};

   m_pointer_listener.enter = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 serial, wl_surface* surface, wl_fixed_t surface_x,
                                 wl_fixed_t surface_y) {
      auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_enter(serial, surface, surface_x, surface_y);
   };

   m_pointer_listener.leave = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 serial, wl_surface* surface) {
      auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_leave(serial, surface);
   };

   m_pointer_listener.motion = [](void* data, [[maybe_unused]] struct wl_pointer* wl_pointer, u32 time, wl_fixed_t surface_x,
                                  wl_fixed_t surface_y) {
      const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_motion(time, surface_x, surface_y);
   };

   m_pointer_listener.button = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 serial, u32 time, u32 button, u32 state) {
      const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_button(serial, time, button, state);
   };

   m_pointer_listener.axis = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 time, u32 axis, wl_fixed_t value) {
      const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
      display->on_pointer_axis(time, axis, value);
   };

   m_pointer_listener.frame = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer) {
      [[maybe_unused]] const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointer_listener.axis_source = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 /*axis_source*/) {
      [[maybe_unused]] const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointer_listener.axis_stop = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 /*time*/, u32 /*axis*/) {
      [[maybe_unused]] const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointer_listener.axis_discrete = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 /*axis*/, i32 /*discrete*/) {
      [[maybe_unused]] const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointer_listener.axis_value120 = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 /*axis*/, i32 /*value120*/) {
      [[maybe_unused]] const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_pointer_listener.axis_relative_direction = [](void* data, [[maybe_unused]] wl_pointer* wl_pointer, u32 /*axis*/, u32 /*direction*/) {
      [[maybe_unused]] const auto* display = static_cast<Display*>(data);
      assert(display->m_pointer == wl_pointer);
   };

   m_relative_pointer_listener.relative_motion = [](void* data, [[maybe_unused]] zwp_relative_pointer_v1* zwp_relative_pointer_v1,
                                                    u32 utime_hi, u32 utime_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel,
                                                    wl_fixed_t dy_unaccel) {
      const auto* display = static_cast<Display*>(data);
      assert(display->m_relative_pointer == zwp_relative_pointer_v1);
      display->on_pointer_relative_motion(utime_hi, utime_lo, dx, dy, dx_unaccel, dy_unaccel);
   };

   m_keyboard_listener.keymap = [](void* data, [[maybe_unused]] wl_keyboard* /*wl_keyboard*/, u32 format, i32 fd, u32 size) {
      auto* display = static_cast<Display*>(data);
      display->on_keymap(format, fd, size);
   };

   m_keyboard_listener.enter = [](void* data, wl_keyboard* /*wl_keyboard*/, u32 serial, wl_surface* surface, wl_array* keys) {
      auto* display = static_cast<Display*>(data);
      display->on_keyboard_enter(serial, surface, keys);
   };

   m_keyboard_listener.leave = [](void* /*data*/, wl_keyboard* /*wl_keyboard*/, u32 /*serial*/, wl_surface* /*surface*/) {
      // TODO: Handle
   };

   m_keyboard_listener.key = [](void* data, [[maybe_unused]] wl_keyboard* wl_keyboard, u32 serial, u32 time, u32 key, u32 state) {
      const auto* display = static_cast<Display*>(data);
      display->on_key(serial, time, key, state);
   };

   m_keyboard_listener.modifiers = [](void* data, wl_keyboard* /*wl_keyboard*/, u32 /*serial*/, const u32 mods_depressed,
                                      const u32 mods_latched, const u32 mods_locked, const u32 group) {
      const auto* display = static_cast<Display*>(data);
      xkb_state_update_mask(display->m_xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
   };

   m_keyboard_listener.repeat_info = [](void* /*data*/, wl_keyboard* /*wl_keyboard*/, const i32 /*rate*/, const i32 /*delay*/) {
      // TODO: Handle
   };


   wl_registry_add_listener(m_registry, &m_registry_listener, this);
   wl_display_roundtrip(m_display);
}

Display::~Display()
{
   if (m_xkb_state != nullptr) {
      xkb_state_unref(m_xkb_state);
   }
   if (m_xkb_keymap != nullptr) {
      xkb_keymap_unref(m_xkb_keymap);
   }
   if (m_decoration_manager != nullptr) {
      zxdg_decoration_manager_v1_destroy(m_decoration_manager);
   }
   if (m_cursor_shape_device != nullptr) {
      wp_cursor_shape_device_v1_destroy(m_cursor_shape_device);
   }
   if (m_cursor_shape_manager != nullptr) {
      wp_cursor_shape_manager_v1_destroy(m_cursor_shape_manager);
   }
   if (m_pointer != nullptr) {
      wl_pointer_destroy(m_pointer);
   }
   if (m_relative_pointer != nullptr) {
      zwp_relative_pointer_v1_destroy(m_relative_pointer);
   }
   if (m_keyboard != nullptr) {
      wl_keyboard_destroy(m_keyboard);
   }
   zwp_relative_pointer_manager_v1_destroy(m_relative_pointer_manager);
   zwp_pointer_constraints_v1_destroy(m_pointer_constraints);
   wl_seat_destroy(m_seat);
   xkb_context_unref(m_xkb_context);
   xdg_wm_base_destroy(m_wm_base);
   wl_compositor_destroy(m_compositor);
   wl_registry_destroy(m_registry);
   wl_display_disconnect(m_display);
}

void Display::on_register_global(const u32 name, const std::string_view interface, u32 /*version*/)
{
   if (interface == wl_compositor_interface.name) {
      m_compositor = static_cast<wl_compositor*>(wl_registry_bind(m_registry, name, &wl_compositor_interface, 4));
      return;
   }
   if (interface == xdg_wm_base_interface.name) {
      m_wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(m_registry, name, &xdg_wm_base_interface, 1));
      xdg_wm_base_add_listener(m_wm_base, &m_wm_base_listener, this);
   }
   if (interface == wl_seat_interface.name) {
      m_seat = static_cast<wl_seat*>(wl_registry_bind(m_registry, name, &wl_seat_interface, 7));
      wl_seat_add_listener(m_seat, &m_seat_listener, this);
   }
   if (interface == zwp_pointer_constraints_v1_interface.name) {
      m_pointer_constraints =
         static_cast<zwp_pointer_constraints_v1*>(wl_registry_bind(m_registry, name, &zwp_pointer_constraints_v1_interface, 1));
   }
   if (interface == zwp_relative_pointer_manager_v1_interface.name) {
      m_relative_pointer_manager =
         static_cast<zwp_relative_pointer_manager_v1*>(wl_registry_bind(m_registry, name, &zwp_relative_pointer_manager_v1_interface, 1));
   }
   if (interface == wp_cursor_shape_manager_v1_interface.name) {
      m_cursor_shape_manager =
         static_cast<wp_cursor_shape_manager_v1*>(wl_registry_bind(m_registry, name, &wp_cursor_shape_manager_v1_interface, 1));
   }
   if (interface == zxdg_decoration_manager_v1_interface.name) {
      m_decoration_manager =
         static_cast<zxdg_decoration_manager_v1*>(wl_registry_bind(m_registry, name, &zxdg_decoration_manager_v1_interface, 1));
   }
}

void Display::on_xdg_ping(const u32 serial) const
{
   xdg_wm_base_pong(m_wm_base, serial);
}

void Display::on_seat_capabilities(const u32 capabilities)
{
   if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0) {
      m_pointer = wl_seat_get_pointer(m_seat);
      wl_pointer_add_listener(m_pointer, &m_pointer_listener, this);

      if (m_relative_pointer_manager) {
         m_relative_pointer = zwp_relative_pointer_manager_v1_get_relative_pointer(m_relative_pointer_manager, m_pointer);
         zwp_relative_pointer_v1_add_listener(m_relative_pointer, &m_relative_pointer_listener, this);
      }
      if (m_cursor_shape_manager != nullptr) {
         m_cursor_shape_device = wp_cursor_shape_manager_v1_get_pointer(m_cursor_shape_manager, m_pointer);
      }
   }

   if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0) {
      m_keyboard = wl_seat_get_keyboard(m_seat);
      wl_keyboard_add_listener(m_keyboard, &m_keyboard_listener, this);
   }
}

void Display::on_pointer_enter(const u32 serial, wl_surface* surface, const i32 x, const i32 y)
{
   if (not m_surface_map.contains(surface))
      return;

   auto pointer_surface = m_pointer_surface.access();

   *pointer_surface = m_surface_map.at(surface);
   if (*pointer_surface == nullptr)
      return;

   (*pointer_surface)->m_pointer_serial = serial;
   (*pointer_surface)->event_OnMouseEnter.publish(Vector2{static_cast<float>(x) / 256.0f, static_cast<float>(y) / 256.0f});
}

void Display::on_pointer_leave(u32 /*serial*/, wl_surface* surface)
{
   if (not m_surface_map.contains(surface))
      return;

   auto pointer_surface = m_pointer_surface.access();
   if (*pointer_surface == nullptr)
      return;

   assert(*pointer_surface == m_surface_map.at(surface));

   (*pointer_surface)->event_OnMouseLeave.publish();

   *pointer_surface = nullptr;
}

void Display::on_pointer_motion(u32 /*time*/, const i32 x, const i32 y) const
{
   auto pointer_surface = m_pointer_surface.read_access();

   if (*pointer_surface == nullptr)
      return;

   (*pointer_surface)->event_OnMouseMove.publish(Vector2{static_cast<float>(x) / 256.0f, static_cast<float>(y) / 256.0f});
}

void Display::dispatch_messages()
{
   for (auto& surface : m_surface_map | std::views::values) {
      surface->tick();
   }
   wl_display_roundtrip(m_display);
   wl_display_dispatch_pending(m_display);
}

std::shared_ptr<ISurface> Display::create_surface(StringView title, Vector2u dimensions, WindowAttributeFlags flags)
{
   return std::make_shared<Surface>(*this, title, dimensions, flags);
}

void Display::register_surface(wl_surface* wayland_surface, Surface* surface)
{
   m_surface_map.emplace(wayland_surface, surface);
}

void Display::on_pointer_relative_motion(u32 /*utime_hi*/, u32 /*utime_lo*/, const i32 dx, const i32 dy, i32 /*dx_unaccel*/,
                                         i32 /*dy_unaccel*/) const
{
   auto pointer_surface = m_pointer_surface.read_access();
   if (*pointer_surface == nullptr)
      return;
   (*pointer_surface)->event_OnMouseRelativeMove.publish(Vector2{static_cast<float>(dx) / 256.0f, static_cast<float>(dy) / 256.0f});
}

void Display::on_pointer_axis(u32 /*time*/, u32 /*axis*/, i32 value) const
{
   auto pointer_surface = m_pointer_surface.read_access();
   if (*pointer_surface == nullptr)
      return;
   (*pointer_surface)->event_OnMouseWheelTurn.publish(static_cast<float>(value) / 2560.0f);
}

void Display::on_pointer_button(const u32 /*serial*/, const u32 /*time*/, const u32 button, const u32 state) const
{
   auto pointer_surface = m_pointer_surface.read_access();
   if (*pointer_surface == nullptr)
      return;

   if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
      (*pointer_surface)->event_OnMouseButtonIsPressed.publish(map_button(button));
   } else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
      (*pointer_surface)->event_OnMouseButtonIsReleased.publish(map_button(button));
   }
}

void Display::on_keyboard_enter(const u32 /*serial*/, wl_surface* surface, wl_array* /*wls*/)
{
   if (not m_surface_map.contains(surface))
      return;

   auto keyboard_surface = m_keyboard_surface.access();
   *keyboard_surface = m_surface_map.at(surface);
}

void Display::on_keymap(const u32 format, const i32 fd, const u32 size)
{
   if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
      return;

   auto* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
   assert(mapped != MAP_FAILED);

   m_xkb_keymap =
      xkb_keymap_new_from_string(m_xkb_context, static_cast<const char*>(mapped), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
   m_xkb_state = xkb_state_new(m_xkb_keymap);

   munmap(mapped, size);
   close(fd);
}

void Display::on_key(const u32 /*serial*/, const u32 /*time*/, const u32 key, const u32 state) const
{
   auto keyboard_surface = m_keyboard_surface.read_access();
   if (*keyboard_surface == nullptr)
      return;

   if ((*keyboard_surface)->m_keyboard_input_mode & KeyboardInputMode::Text && state == WL_KEYBOARD_KEY_STATE_PRESSED) {
      char data[128];
      const i32 count = xkb_state_key_get_utf8(m_xkb_state, key + 8, data, 128);
      if (count > 0) {
         const char* buff_ptr = data;
         Rune rune = decode_rune_from_buffer(buff_ptr, buff_ptr + count);
         (*keyboard_surface)->event_OnTextInput.publish(rune);
      }
   }

   if ((*keyboard_surface)->m_keyboard_input_mode & KeyboardInputMode::Direct) {
      if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
         (*keyboard_surface)->event_OnKeyIsPressed.publish(map_key(key));
      } else if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
         (*keyboard_surface)->event_OnKeyIsReleased.publish(map_key(key));
      }
   }
}

void Display::on_destroyed_surface(Surface* surface)
{
   {
      auto pointer_surface = m_pointer_surface.access();
      if (*pointer_surface == surface) {
         *pointer_surface = nullptr;
      }
   }

   {
      auto keyboard_surface = m_keyboard_surface.access();
      if (*keyboard_surface == surface) {
         *keyboard_surface = nullptr;
      }
   }

   m_surface_map.erase(surface->surface());
}

}// namespace triglav::desktop::wayland
