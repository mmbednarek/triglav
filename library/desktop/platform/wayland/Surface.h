#pragma once

#include "Display.h"
#include "api/XdgShell.h"

#include <optional>

namespace triglav::desktop {

class Display;

class Surface : public ISurface
{
   friend Display;

 public:
   explicit Surface(Display& display, Dimension dimension);
   ~Surface() override;

   void on_configure(uint32_t serial);
   void on_toplevel_configure(int32_t width, int32_t height, wl_array* states);
   void on_toplevel_close() const;
   void lock_cursor() override;
   void unlock_cursor() override;
   void hide_cursor() const override;
   void add_event_listener(ISurfaceEventListener* eventListener) override;
   void tick();
   [[nodiscard]] bool is_cursor_locked() const override;
   [[nodiscard]] Dimension dimension() const override;

   [[nodiscard]] constexpr Display& display() const noexcept
   {
      return m_display;
   }

   [[nodiscard]] constexpr wl_surface* surface() const noexcept
   {
      return m_surface;
   }

 private:
   [[nodiscard]] ISurfaceEventListener& event_listener() const;

   Display& m_display;
   wl_surface* m_surface{};
   xdg_surface* m_xdgSurface{};
   xdg_surface_listener m_surfaceListener{};
   xdg_toplevel* m_topLevel{};
   xdg_toplevel_listener m_topLevelListener{};
   zwp_locked_pointer_v1* m_lockedPointer{};
   bool m_resizeReady = false;

   ISurfaceEventListener* m_eventListener{};
   uint32_t m_pointerSerial{};

   Dimension m_dimension{};
   std::optional<Dimension> m_pendingDimension{};
};

}// namespace triglav::desktop