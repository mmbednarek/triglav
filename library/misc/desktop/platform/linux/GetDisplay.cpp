#include "wayland/Display.hpp"
#include "x11/Display.hpp"

#include <cstdlib>
#include <string>

namespace triglav::desktop {

std::unique_ptr<IDisplay> get_display()
{
   const std::string sessionName{std::getenv("XDG_SESSION_TYPE")};
   if (sessionName == "wayland") {
      return std::make_unique<wayland::Display>();
   }
   return std::make_unique<x11::Display>();
}

}// namespace triglav::desktop
