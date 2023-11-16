#pragma once

#include "api/XdgShell.h"
#include "Display.h"
#include "graphics_api/PlatformSurface.h"

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
   explicit Surface(Display &display);
   ~Surface();

   void on_configure(uint32_t serial);
   void on_toplevel_configure(int32_t width, int32_t height, wl_array *states);
   void on_toplevel_close();
   void lock_cursor();
   void unlock_cursor();
   [[nodiscard]] bool is_cursor_locked() const;

   [[nodiscard]] graphics_api::Surface to_grahics_surface() const;

   [[nodiscard]] Dimension dimension() const;
   [[nodiscard]] SurfaceMessage message();

   [[nodiscard]] constexpr wl_surface *surface() const noexcept
   {
      return m_surface;
   }

 private:
   Display &m_display;
   wl_surface *m_surface{};
   xdg_surface *m_xdgSurface{};
   xdg_surface_listener m_surfaceListener{};
   xdg_toplevel *m_topLevel{};
   xdg_toplevel_listener m_topLevelListener{};
   zwp_locked_pointer_v1 *m_lockedPointer{};

   Dimension m_dimension{};
   Dimension m_penndingDimension{};
   PendingState m_resizeState{};
   PendingState m_closeState{};
};

}// namespace wayland