#pragma once

#include <Windows.h>

#include "IDisplay.hpp"

namespace triglav::desktop {

class Display : public IDisplay
{
 public:
   explicit Display(HINSTANCE instance);

   void dispatch_messages() const override;
   std::shared_ptr<ISurface> create_surface(int width, int height) override;

 private:
   HINSTANCE m_instance;
};

}// namespace triglav::desktop