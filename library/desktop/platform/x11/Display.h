#pragma once

#include <X11/Xlib.h>
#include <map>

#include "triglav/desktop/IDisplay.hpp"

namespace triglav::desktop::x11 {

class Display final : public IDisplay
{
public:
  Display();
  ~Display() override;

  void dispatch_messages() const override;
  std::shared_ptr<ISurface> create_surface(int width, int height) override;

private:
  ::Display *m_display{};
  Window m_rootWindow{};
  std::map<Window, std::shared_ptr<ISurface>> m_surfaces;
};

}