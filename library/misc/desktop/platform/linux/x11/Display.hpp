#pragma once

#include <X11/Xlib.h>
#include <map>

#include "triglav/desktop/IDisplay.hpp"

#include "Mouse.hpp"

namespace triglav::desktop::x11 {

class Surface;

class Display final : public IDisplay
{
 public:
   using Self = Display;

   Display();
   ~Display() override;

   void dispatch_messages() override;
   std::shared_ptr<ISurface> create_surface(StringView title, Vector2u dimensions, WindowAttributeFlags flags) override;

   [[nodiscard]] constexpr ::Display* x11_display() const
   {
      return m_display;
   }

   void map_surface(::Window window, const std::weak_ptr<ISurface>& surface);

 private:
   void on_mouse_move(float x, float y);
   std::shared_ptr<Surface> surface_by_window(Window wnd_handle);

   ::Display* m_display{};
   Window m_root_window{};
   std::map<Window, std::weak_ptr<ISurface>> m_surfaces;
   Mouse m_mouse;
   Atom m_wm_delete_atom;

   TG_SINK(Mouse, OnMouseMove);
};

}// namespace triglav::desktop::x11