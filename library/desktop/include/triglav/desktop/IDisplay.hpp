#pragma once

#include <memory>

#include "triglav/Math.hpp"
#include "triglav/String.hpp"

#include "ISurface.hpp"

namespace triglav::desktop {

class ISurface;

class IDisplay
{
 public:
   IDisplay() = default;
   virtual ~IDisplay() = default;

   IDisplay(const IDisplay& display) = delete;
   IDisplay& operator=(const IDisplay& display) = delete;
   IDisplay(IDisplay&& display) noexcept = delete;
   IDisplay& operator=(IDisplay&& display) noexcept = delete;

   virtual void dispatch_messages() = 0;
   virtual std::shared_ptr<ISurface> create_surface(StringView title, Vector2u dimensions, WindowAttributeFlags flags) = 0;
};

std::unique_ptr<IDisplay> get_display();

}// namespace triglav::desktop