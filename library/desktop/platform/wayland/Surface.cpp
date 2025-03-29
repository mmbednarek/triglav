#include "Surface.h"

#include "Display.h"
#include "api/CursorShape.h"

#include <cassert>

namespace triglav::desktop {

Surface::Surface(Display& display, const std::string_view title, const Dimension dimension, const WindowAttributeFlags attributes) :
    m_display(display),
    m_surface(wl_compositor_create_surface(display.compositor())),
    m_xdgSurface(xdg_wm_base_get_xdg_surface(display.wm_base(), m_surface)),
    m_topLevel(xdg_surface_get_toplevel(m_xdgSurface)),
    m_dimension(dimension),
    m_attributes(attributes)
{
   m_surfaceListener.configure = [](void* data, [[maybe_unused]] xdg_surface* xdg_surface, const uint32_t serial) {
      auto* surface = static_cast<Surface*>(data);
      assert(surface->m_xdgSurface == xdg_surface);
      surface->on_configure(serial);
   };
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

   xdg_surface_add_listener(m_xdgSurface, &m_surfaceListener, this);
   xdg_toplevel_add_listener(m_topLevel, &m_topLevelListener, this);
   xdg_toplevel_set_title(m_topLevel, title.data());
   xdg_toplevel_set_app_id(m_topLevel, "triglav-surface");

   m_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(m_display.m_decorationManager, m_topLevel);

   if (attributes & WindowAttribute::ShowDecorations) {
      zxdg_toplevel_decoration_v1_set_mode(m_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
   } else {
      zxdg_toplevel_decoration_v1_set_mode(m_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
   }

   wl_surface_commit(m_surface);
   wl_display_roundtrip(m_display.display());
   wl_surface_commit(m_surface);

   display.register_surface(m_surface, this);
}

Surface::~Surface()
{
   m_display.on_destroyed_surface(this);

   if (m_decoration != nullptr) {
      zxdg_toplevel_decoration_v1_destroy(m_decoration);
   }
   xdg_toplevel_destroy(m_topLevel);
   xdg_surface_destroy(m_xdgSurface);
   wl_surface_destroy(m_surface);
}

void Surface::on_configure(const uint32_t serial)
{
   xdg_surface_ack_configure(m_xdgSurface, serial);

   if (m_pendingDimension.has_value()) {
      m_resizeReady = true;
   }
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
   default:
      return;
   }
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

}// namespace triglav::desktop