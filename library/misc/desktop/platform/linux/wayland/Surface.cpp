#include "Surface.hpp"

#include "Display.hpp"
#include "api/CursorShape.h"

#include <cassert>

namespace triglav::desktop::wayland {

using namespace string_literals;

Surface::Surface(Display& display, const StringView title, const Dimension dimension, const WindowAttributeFlags attributes) :
    m_display(display),
    m_surface(wl_compositor_create_surface(display.compositor())),
    m_xdgSurface(xdg_wm_base_get_xdg_surface(display.wm_base(), m_surface)),
    m_dimension(dimension),
    m_attributes(attributes)
{
   m_surfaceListener.configure = [](void* data, [[maybe_unused]] xdg_surface* xdg_surface, const uint32_t serial) {
      auto* surface = static_cast<Surface*>(data);
      assert(surface->m_xdgSurface == xdg_surface);
      surface->on_configure(serial);
   };
   xdg_surface_add_listener(m_xdgSurface, &m_surfaceListener, this);

   m_topLevelListener.configure = [](void* data, [[maybe_unused]] xdg_toplevel* xdg_toplevel, const int32_t width, const int32_t height,
                                     wl_array* states) {
      auto* surface = static_cast<Surface*>(data);
      assert(surface->m_topLevel == xdg_toplevel);
      surface->on_toplevel_configure(width, height, states);
   };
   m_topLevelListener.close = [](void* data, [[maybe_unused]] xdg_toplevel* xdg_toplevel) {
      const auto* surface = static_cast<Surface*>(data);
      assert(surface->m_topLevel == xdg_toplevel);
      surface->on_toplevel_close();
   };

   m_topLevel = xdg_surface_get_toplevel(m_xdgSurface);
   xdg_toplevel_add_listener(m_topLevel, &m_topLevelListener, this);
   xdg_toplevel_set_title(m_topLevel, title.data());
   xdg_toplevel_set_app_id(m_topLevel, "triglav-surface");

   if (m_display.m_decorationManager != nullptr) {
      m_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(m_display.m_decorationManager, m_topLevel);
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

Surface::Surface(Display& display, ISurface& parentSurface, const Dimension dimension, const Vector2 offset,
                 const WindowAttributeFlags attributes) :
    m_display(display),
    m_surface(wl_compositor_create_surface(display.compositor())),
    m_xdgSurface(xdg_wm_base_get_xdg_surface(display.wm_base(), m_surface)),
    m_dimension(dimension),
    m_attributes(attributes)
{
   m_surfaceListener.configure = [](void* data, [[maybe_unused]] xdg_surface* xdg_surface, const uint32_t serial) {
      auto* surface = static_cast<Surface*>(data);
      assert(surface->m_xdgSurface == xdg_surface);
      surface->on_configure(serial);
   };
   xdg_surface_add_listener(m_xdgSurface, &m_surfaceListener, this);

   m_popupListener.configure = [](void* data, xdg_popup* /*popup*/, int x, int y, int width, int height) {
      [[maybe_unused]] auto* surface = static_cast<Surface*>(data);
      surface->on_popup_configure({x, y}, {width, height});
   };
   m_popupListener.popup_done = [](void* data, xdg_popup* /*popup*/) {
      [[maybe_unused]] auto* surface = static_cast<Surface*>(data);
      surface->on_popup_done();
   };
   m_popupListener.repositioned = [](void* data, xdg_popup* /*popup*/, u32 token) {
      [[maybe_unused]] auto* surface = static_cast<Surface*>(data);
      surface->on_popup_repositioned(token);
   };

   auto* positioner = xdg_wm_base_create_positioner(m_display.wm_base());
   xdg_positioner_set_anchor_rect(positioner, static_cast<int>(offset.x), static_cast<int>(offset.y), parentSurface.dimension().x,
                                  parentSurface.dimension().y);
   xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
   xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
   xdg_positioner_set_size(positioner, m_dimension.x, m_dimension.y);
   xdg_positioner_set_constraint_adjustment(positioner,
                                            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y);

   m_popup = xdg_surface_get_popup(m_xdgSurface, dynamic_cast<Surface&>(parentSurface).m_xdgSurface, positioner);
   xdg_popup_add_listener(m_popup, &m_popupListener, this);

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
   if (m_topLevel != nullptr) {
      xdg_toplevel_destroy(m_topLevel);
   }
   xdg_surface_destroy(m_xdgSurface);
   wl_surface_destroy(m_surface);
}

void Surface::on_configure(const uint32_t serial)
{
   xdg_surface_ack_configure(m_xdgSurface, serial);

   if (m_pendingDimension.has_value()) {
      m_resizeReady = true;
   }
   m_isConfigured = true;
}

void Surface::on_toplevel_configure(const int32_t width, const int32_t height, wl_array* /*states*/)
{
   if (width == 0 || height == 0)
      return;

   if (width == m_dimension.x && height == m_dimension.y)
      return;

   m_pendingDimension = Dimension{width, height};
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

void Surface::lock_cursor()
{
   if (m_lockedPointer != nullptr) {
      zwp_locked_pointer_v1_destroy(m_lockedPointer);
   }
   m_lockedPointer = zwp_pointer_constraints_v1_lock_pointer(m_display.pointer_constraints(), m_surface, m_display.pointer(), nullptr, 1);
}

void Surface::unlock_cursor()
{
   if (m_lockedPointer != nullptr) {
      zwp_locked_pointer_v1_destroy(m_lockedPointer);
      m_lockedPointer = nullptr;
   }
}

void Surface::hide_cursor() const
{
   wl_pointer_set_cursor(m_display.pointer(), m_pointerSerial, nullptr, 0, 0);
}

bool Surface::is_cursor_locked() const
{
   return m_lockedPointer != nullptr;
}

Dimension Surface::dimension() const
{
   return m_dimension;
}

void Surface::set_cursor_icon(const CursorIcon icon)
{
   switch (icon) {
   case CursorIcon::Arrow:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursorShapeDevice, m_pointerSerial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
      break;
   case CursorIcon::Hand:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursorShapeDevice, m_pointerSerial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);
      break;
   case CursorIcon::Move:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursorShapeDevice, m_pointerSerial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE);
      break;
   case CursorIcon::Wait:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursorShapeDevice, m_pointerSerial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT);
      break;
   case CursorIcon::Edit:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursorShapeDevice, m_pointerSerial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT);
      break;
   case CursorIcon::ResizeHorizontal:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursorShapeDevice, m_pointerSerial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE);
      break;
   case CursorIcon::ResizeVertical:
      wp_cursor_shape_device_v1_set_shape(m_display.m_cursorShapeDevice, m_pointerSerial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE);
      break;
   default:
      return;
   }
}

void Surface::set_keyboard_input_mode(const KeyboardInputModeFlags mode)
{
   m_keyboardInputMode = mode;
}

std::shared_ptr<ISurface> Surface::create_popup(const Vector2u dimensions, const Vector2 offset, const WindowAttributeFlags flags)
{
   return std::make_shared<Surface>(m_display, *this, dimensions, offset, flags);
}

void Surface::tick()
{
   if (m_resizeReady) {
      m_dimension = *m_pendingDimension;
      m_pendingDimension.reset();
      m_resizeReady = false;
      event_OnResize.publish(Vector2i{m_dimension.x, m_dimension.y});

      wl_surface_commit(m_surface);
   }
}

}// namespace triglav::desktop::wayland