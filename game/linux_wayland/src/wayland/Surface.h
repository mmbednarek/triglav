#pragma once

#include "api/XdgShell.h"
#include "Display.h"
#include "graphics_api/PlatformSurface.h"

#include <optional>

namespace wayland {

class Display;

struct Dimension
{
   int width;
   int height;
};

class Surface
{
   friend Display;
 public:
   explicit Surface(Display &display);
   ~Surface();

   void on_configure(uint32_t serial);
   void on_toplevel_configure(int32_t width, int32_t height, wl_array *states);
   void on_toplevel_close() const;
   void lock_cursor();
   void unlock_cursor();
   void hide_cursor() const;
   void set_event_listener(ISurfaceEventListener *eventListener);
   [[nodiscard]] bool is_cursor_locked() const;

   [[nodiscard]] graphics_api::Surface to_grahics_surface() const;

   [[nodiscard]] Dimension dimension() const;

   [[nodiscard]] constexpr wl_surface *surface() const noexcept
   {
      return m_surface;
   }

 private:
   [[nodiscard]] ISurfaceEventListener& event_listener() const;

   Display &m_display;
   wl_surface *m_surface{};
   xdg_surface *m_xdgSurface{};
   xdg_surface_listener m_surfaceListener{};
   xdg_toplevel *m_topLevel{};
   xdg_toplevel_listener m_topLevelListener{};
   zwp_locked_pointer_v1 *m_lockedPointer{};

   ISurfaceEventListener *m_eventListener{};
   uint32_t m_pointerSerial{};

   Dimension m_dimension{};
   std::optional<Dimension> m_penndingDimension{};
};

}// namespace wayland