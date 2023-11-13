#include "Surface.h"

#include "Display.h"

#include <cassert>

namespace wayland {

Surface::Surface(const Display &display) :
    m_display(display.display()),
    m_surface(wl_compositor_create_surface(display.compositor())),
    m_xdgSurface(xdg_wm_base_get_xdg_surface(display.wm_base(), m_surface)),
    m_topLevel(xdg_surface_get_toplevel(m_xdgSurface))
{
   m_surfaceListener.configure = [](void *data, xdg_surface *xdg_surface, const uint32_t serial) {
      auto *surface = static_cast<Surface *>(data);
      assert(surface->m_xdgSurface == xdg_surface);
      surface->on_configure(serial);
   };
   m_topLevelListener.configure = [](void *data, xdg_toplevel *xdg_toplevel, const int32_t width,
                                     const int32_t height, wl_array *states) {
      auto *surface = static_cast<Surface *>(data);
      assert(surface->m_topLevel == xdg_toplevel);
      surface->on_toplevel_configure(width, height, states);
   };
   m_topLevelListener.close = [](void *data, xdg_toplevel *xdg_toplevel) {
      auto *surface = static_cast<Surface *>(data);
      assert(surface->m_topLevel == xdg_toplevel);
      surface->on_toplevel_close();
   };

   xdg_surface_add_listener(m_xdgSurface, &m_surfaceListener, this);
   xdg_toplevel_add_listener(m_topLevel, &m_topLevelListener, this);
   xdg_toplevel_set_title(m_topLevel, "Example client");
   xdg_toplevel_set_app_id(m_topLevel, "example-client");
   wl_surface_commit(m_surface);
}

Surface::~Surface()
{
   xdg_toplevel_destroy(m_topLevel);
   xdg_surface_destroy(m_xdgSurface);
   wl_surface_destroy(m_surface);
}

void Surface::on_configure(const uint32_t serial)
{
   if (m_resizeState == PendingState::Requested) {
      m_resizeState = PendingState::Responded;
   }
   xdg_surface_ack_configure(m_xdgSurface, serial);
}

void Surface::on_toplevel_configure(const int32_t width, const int32_t height, wl_array * /*states*/)
{
   if (width == 0 || height == 0)
      return;

   if (width == m_dimension.width && height == m_dimension.height)
      return;

   m_resizeState       = PendingState::Requested;
   m_penndingDimension = {width, height};
}

void Surface::on_toplevel_close()
{
   m_closeState = PendingState::Requested;
}

graphics_api::Surface Surface::to_grahics_surface() const
{
   return graphics_api::Surface{.display = m_display, .surface = m_surface};
}

Dimension Surface::dimension() const
{
   return m_dimension;
}

SurfaceMessage Surface::message()
{
   if (m_resizeState == PendingState::Responded) {
      m_resizeState = PendingState::None;
      m_dimension   = m_penndingDimension;
      return SurfaceMessage::Resize;
   }
   if (m_closeState != PendingState::None) {
      m_closeState = PendingState::None;
      return SurfaceMessage::Close;
   }
   return SurfaceMessage::None;
}


}// namespace wayland