#include "Surface.hpp"

#include "Display.hpp"
#include "api/CursorShape.h"

#include <cassert>

namespace triglav::desktop::wayland {

using namespace string_literals;

namespace {

Key map_key(const u32 key)
{
   switch (key) {
   case 1:
      return Key::Escape;
   case 2:
      return Key::Digit1;
   case 3:
      return Key::Digit2;
   case 4:
      return Key::Digit3;
   case 5:
      return Key::Digit4;
   case 6:
      return Key::Digit5;
   case 7:
      return Key::Digit6;
   case 8:
      return Key::Digit7;
   case 9:
      return Key::Digit8;
   case 10:
      return Key::Digit9;
   case 11:
      return Key::Digit0;
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
   case 19:
      return Key::R;
   case 20:
      return Key::T;
   case 21:
      return Key::Y;
   case 22:
      return Key::U;
   case 23:
      return Key::I;
   case 24:
      return Key::O;
   case 25:
      return Key::P;
   case 26:
      return Key::LeftBrace;
   case 27:
      return Key::RightBrace;
   case 28:
      return Key::Enter;
   case 29:
      return Key::Control;
   case 30:
      return Key::A;
   case 31:
      return Key::S;
   case 32:
      return Key::D;
   case 33:
      return Key::F;
   case 34:
      return Key::G;
   case 35:
      return Key::H;
   case 36:
      return Key::J;
   case 37:
      return Key::K;
   case 38:
      return Key::L;
   case 39:
      return Key::Semicolon;
   case 40:
      return Key::Quote;
   case 41:
      return Key::Tilde;
   case 42:
      return Key::Shift;
   case 44:
      return Key::Z;
   case 45:
      return Key::X;
   case 46:
      return Key::C;
   case 47:
      return Key::V;
   case 48:
      return Key::B;
   case 49:
      return Key::N;
   case 50:
      return Key::M;
   case 51:
      return Key::Comma;
   case 52:
      return Key::Dot;
   case 53:
      return Key::Slash;
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
   case 111:
      return Key::Delete;
   }

   return Key::Unknown;
}

}// namespace

Surface::Surface(Display& display, const StringView title, const Dimension dimension, const WindowAttributeFlags attributes) :
    m_display(display),
    m_surface(wl_compositor_create_surface(display.compositor())),
    m_xdg_surface(xdg_wm_base_get_xdg_surface(display.wm_base(), m_surface)),
    m_dimension(dimension),
    m_attributes(attributes)
{
   m_surface_listener.configure = [](void* data, [[maybe_unused]] xdg_surface* xdg_surface, const uint32_t serial) {
      auto* surface = static_cast<Surface*>(data);
      assert(surface->m_xdg_surface == xdg_surface);
      surface->on_configure(serial);
   };
   xdg_surface_add_listener(m_xdg_surface, &m_surface_listener, this);

   m_top_level_listener.configure = [](void* data, [[maybe_unused]] xdg_toplevel* xdg_toplevel, const int32_t width, const int32_t height,
                                       wl_array* states) {
      auto* surface = static_cast<Surface*>(data);
      assert(surface->m_top_level == xdg_toplevel);
      surface->on_toplevel_configure(width, height, states);
   };
   m_top_level_listener.close = [](void* data, [[maybe_unused]] xdg_toplevel* xdg_toplevel) {
      const auto* surface = static_cast<Surface*>(data);
      assert(surface->m_top_level == xdg_toplevel);
      surface->on_toplevel_close();
   };

   m_top_level = xdg_surface_get_toplevel(m_xdg_surface);
   xdg_toplevel_add_listener(m_top_level, &m_top_level_listener, this);
   xdg_toplevel_set_title(m_top_level, title.data());
   xdg_toplevel_set_app_id(m_top_level, "triglav-surface");

   if (m_display.m_decoration_manager != nullptr) {
      m_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(m_display.m_decoration_manager, m_top_level);
      if (attributes & WindowAttribute::ShowDecorations) {
         zxdg_toplevel_decoration_v1_set_mode(m_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
      } else {
         zxdg_toplevel_decoration_v1_set_mode(m_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
      }
   }

   wl_surface_commit(m_surface);
   wl_display_roundtrip(m_display.display());
   wl_surface_commit(m_surface);

   display.register_surface(m_surface, this);
}

Surface::Surface(Display& display, ISurface& parent_surface, const Dimension dimension, const Vector2 offset,
                 const WindowAttributeFlags attributes) :
    m_display(display),
    m_surface(wl_compositor_create_surface(display.compositor())),
    m_xdg_surface(xdg_wm_base_get_xdg_surface(display.wm_base(), m_surface)),
    m_dimension(dimension),
    m_attributes(attributes)
{
   m_surface_listener.configure = [](void* data, [[maybe_unused]] xdg_surface* xdg_surface, const uint32_t serial) {
      auto* surface = static_cast<Surface*>(data);
      assert(surface->m_xdg_surface == xdg_surface);
      surface->on_configure(serial);
   };
   xdg_surface_add_listener(m_xdg_surface, &m_surface_listener, this);

   m_popup_listener.configure = [](void* data, xdg_popup* /*popup*/, int x, int y, int width, int height) {
      [[maybe_unused]] auto* surface = static_cast<Surface*>(data);
      surface->on_popup_configure({x, y}, {width, height});
   };
   m_popup_listener.popup_done = [](void* data, xdg_popup* /*popup*/) {
      [[maybe_unused]] auto* surface = static_cast<Surface*>(data);
      surface->on_popup_done();
   };
   m_popup_listener.repositioned = [](void* data, xdg_popup* /*popup*/, u32 token) {
      [[maybe_unused]] auto* surface = static_cast<Surface*>(data);
      surface->on_popup_repositioned(token);
   };

   auto* positioner = xdg_wm_base_create_positioner(m_display.wm_base());
   xdg_positioner_set_anchor_rect(positioner, static_cast<int>(offset.x), static_cast<int>(offset.y), parent_surface.dimension().x,
                                  parent_surface.dimension().y);
   xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
   xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
   xdg_positioner_set_size(positioner, m_dimension.x, m_dimension.y);
   xdg_positioner_set_constraint_adjustment(positioner,
                                            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y);

   m_popup = xdg_surface_get_popup(m_xdg_surface, dynamic_cast<Surface&>(parent_surface).m_xdg_surface, positioner);
   xdg_popup_add_listener(m_popup, &m_popup_listener, this);

   wl_surface_commit(m_surface);
   wl_display_roundtrip(m_display.display());
   wl_surface_commit(m_surface);

   xdg_positioner_destroy(positioner);

   display.register_surface(m_surface, this);
}

Surface::~Surface()
{
   m_display.on_destroyed_surface(this);

   if (m_popup != nullptr) {
      xdg_popup_destroy(m_popup);
   }
   if (m_decoration != nullptr) {
      zxdg_toplevel_decoration_v1_destroy(m_decoration);
   }
   if (m_top_level != nullptr) {
      xdg_toplevel_destroy(m_top_level);
   }
   xdg_surface_destroy(m_xdg_surface);
   wl_surface_destroy(m_surface);
}

void Surface::on_configure(const uint32_t serial)
{
   xdg_surface_ack_configure(m_xdg_surface, serial);

   if (m_pending_dimension.has_value()) {
      m_resize_ready = true;
   }
   m_is_configured = true;
}

void Surface::on_toplevel_configure(const int32_t width, const int32_t height, wl_array* /*states*/)
{
   if (width == 0 || height == 0)
      return;

   if (width == m_dimension.x && height == m_dimension.y)
      return;

   m_pending_dimension = Dimension{width, height};
}

void Surface::on_toplevel_close() const
{
   event_OnClose.publish();
}

void Surface::on_popup_configure(Vector2i position, Vector2i size)
{
   log_debug("on popup configure (position: ({}, {}), size: ({}, {}))", position.x, position.y, size.x, size.y);
}

void Surface::on_popup_done()
{
   log_debug("on popup done");
}

void Surface::on_popup_repositioned(u32 token)
{
   log_debug("on popup repositioned: {}", token);
}

void Surface::on_key(u32 /*serial*/, u32 /*time*/, const u32 key, const u32 state) const
{
   if (m_keyboard_input_mode & KeyboardInputMode::Text && state == WL_KEYBOARD_KEY_STATE_PRESSED) {
      char data[128];
      const i32 count = xkb_state_key_get_utf8(m_display.m_xkb_state, key + 8, data, 128);
      if (count > 0) {
         const char* buff_ptr = data;
         Rune rune = decode_rune_from_buffer(buff_ptr, buff_ptr + count);
         event_OnTextInput.publish(rune);
      }
   }

   if (m_keyboard_input_mode & KeyboardInputMode::Direct) {
      if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
         event_OnKeyIsPressed.publish(map_key(key));
      } else if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
         event_OnKeyIsReleased.publish(map_key(key));
      }
   }
}

void Surface::on_modifiers(u32 /*serial*/, const u32 mods_depressed, u32 /*mods_latched*/, u32 /*mods_locked*/, u32 /*group*/)
{
   m_modifiers = Modifier::Empty;
   if ((mods_depressed & (1 << 0u)) != 0) {
      m_modifiers |= Modifier::Shift;
   }
   // 1 is caps
   if ((mods_depressed & (1 << 2u)) != 0) {
      m_modifiers |= Modifier::Control;
   }
   if ((mods_depressed & (1 << 3u)) != 0) {
      m_modifiers |= Modifier::Alt;
   }
}

void Surface::lock_cursor()
{
   if (m_locked_pointer != nullptr) {
      zwp_locked_pointer_v1_destroy(m_locked_pointer);
   }
   m_locked_pointer = zwp_pointer_constraints_v1_lock_pointer(m_display.pointer_constraints(), m_surface, m_display.pointer(), nullptr, 1);
}

void Surface::unlock_cursor()
{
   if (m_locked_pointer != nullptr) {
      zwp_locked_pointer_v1_destroy(m_locked_pointer);
      m_locked_pointer = nullptr;
   }
}

void Surface::hide_cursor() const
{
   wl_pointer_set_cursor(m_display.pointer(), m_pointer_serial, nullptr, 0, 0);
}

bool Surface::is_cursor_locked() const
{
   return m_locked_pointer != nullptr;
}

Dimension Surface::dimension() const
{
   return m_dimension;
}

void Surface::set_cursor_icon(const CursorIcon icon)
{
   switch (icon) {
   case CursorIcon::Arrow:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursor_shape_device, m_pointer_serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
      break;
   case CursorIcon::Hand:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursor_shape_device, m_pointer_serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
      break;
   case CursorIcon::Move:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursor_shape_device, m_pointer_serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE);
      break;
   case CursorIcon::Wait:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursor_shape_device, m_pointer_serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT);
      break;
   case CursorIcon::Edit:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursor_shape_device, m_pointer_serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT);
      break;
   case CursorIcon::ResizeHorizontal:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursor_shape_device, m_pointer_serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE);
      break;
   case CursorIcon::ResizeVertical:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursor_shape_device, m_pointer_serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE);
      break;
   default:
      return;
   }
}

void Surface::set_keyboard_input_mode(const KeyboardInputModeFlags mode)
{
   m_keyboard_input_mode = mode;
}

std::shared_ptr<ISurface> Surface::create_popup(const Vector2u dimensions, const Vector2 offset, const WindowAttributeFlags flags)
{
   return std::make_shared<Surface>(m_display, *this, dimensions, offset, flags);
}

ModifierFlags Surface::modifiers() const
{
   return m_modifiers;
}

void Surface::tick()
{
   if (m_resize_ready) {
      m_dimension = *m_pending_dimension;
      m_pending_dimension.reset();
      m_resize_ready = false;
      event_OnResize.publish(Vector2i{m_dimension.x, m_dimension.y});

      wl_surface_commit(m_surface);
   }
}

}// namespace triglav::desktop::wayland