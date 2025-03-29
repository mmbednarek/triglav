#pragma once

#include <windows.h>

#include "IDisplay.hpp"

namespace triglav::desktop {

class Display : public IDisplay
{
 public:
   explicit Display(HINSTANCE instance);

   void dispatch_messages() override;
   std::shared_ptr<ISurface> create_surface(std::string_view title, Vector2u dimensions, WindowAttributeFlags flags) override;

 private:
   HINSTANCE m_instance;
};

}// namespace triglav::desktop