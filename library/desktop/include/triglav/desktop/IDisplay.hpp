#pragma once

#include <memory>

#include "ISurface.hpp"

namespace triglav::desktop {

class ISurface;

class IDisplay {
public:
   virtual ~IDisplay() = default;

   virtual void dispatch_messages() const = 0;
   virtual std::unique_ptr<ISurface> create_surface(int width, int height) = 0;
};

std::unique_ptr<IDisplay> get_display();

}// namespace triglav::desktop