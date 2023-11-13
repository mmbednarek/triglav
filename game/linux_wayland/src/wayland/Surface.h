#pragma once

#include "graphics_api/PlatformSurface.h"
#include "xdg-shell-client-protocol.h"

namespace wayland {

class Display;

struct Dimension
{
   int width;
   int height;
};

enum class PendingState
{
   None,
   Requested,
   Responded
};

enum class SurfaceMessage
{
   None,
   Resize,
   Close
};

class Surface
{
public:
   explicit Surface(const Display &display);
   ~Surface();

   void on_configure(uint32_t serial);
   void on_toplevel_configure(int32_t width, int32_t height, wl_array *states);
   void on_toplevel_close();

   [[nodiscard]] graphics_api::Surface to_grahics_surface() const;

   [[nodiscard]] Dimension dimension() const;
   [[nodiscard]] SurfaceMessage message();

   [[nodiscard]] constexpr wl_surface *surface() const noexcept
   {
      return m_surface;
   }

private:
   wl_display *m_display{};
   wl_surface *m_surface{};
   xdg_surface *m_xdgSurface{};
   xdg_surface_listener m_surfaceListener{};
   xdg_toplevel *m_topLevel{};
   xdg_toplevel_listener m_topLevelListener{};

   Dimension m_dimension{};
   Dimension m_penndingDimension{};
   PendingState m_resizeState{};
   PendingState m_closeState{};
};

}