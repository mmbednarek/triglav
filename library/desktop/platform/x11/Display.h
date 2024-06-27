#pragma once

#include <X11/Xlib.h>
#include <map>

#include "triglav/desktop/IDisplay.hpp"

#include "Mouse.h"

namespace triglav::desktop::x11 {

class Surface;

class Display final : public IDisplay
{
 public:
   Display();
   ~Display() override;

   void dispatch_messages() override;
   void dispatch_messages_blocking() override;
   std::shared_ptr<ISurface> create_surface(int width, int height, WindowAttributeFlags flags) override;

 private:
   void on_mouse_move(float x, float y);
   std::shared_ptr<Surface> surface_by_window(Window wndHandle);

   ::Display* m_display{};
   Window m_rootWindow{};
   std::map<Window, std::weak_ptr<ISurface>> m_surfaces;
   Mouse m_mouse;
   Mouse::OnMouseMoveDel::Sink<Display> m_onMouseMoveSink;
};

}// namespace triglav::desktop::x11