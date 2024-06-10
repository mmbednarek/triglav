#pragma once

#include <X11/Xlib.h>
#include <map>

#include "triglav/desktop/IDisplay.hpp"

namespace triglav::desktop::x11 {

class Surface;

class Display final : public IDisplay
{
 public:
   Display();
   ~Display() override;

   void dispatch_messages() override;
   std::shared_ptr<ISurface> create_surface(int width, int height, WindowAttributeFlags flags) override;

 private:
   std::shared_ptr<Surface> surface_by_window(Window wndHandle);

   ::Display* m_display{};
   Window m_rootWindow{};
   std::map<Window, std::weak_ptr<ISurface>> m_surfaces;
};

}// namespace triglav::desktop::x11