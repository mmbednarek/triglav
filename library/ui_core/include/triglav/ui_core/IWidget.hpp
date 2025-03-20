#pragma once

#include "triglav/Math.hpp"

#include <memory>

namespace triglav::ui_core {

class IWidget
{
 public:
   virtual ~IWidget() = default;

   // Calculates widget's desired size based on parent's size.
   [[nodiscard]] virtual Vector2 desired_size(Vector2 parentSize) const = 0;

   // Adds widget to the current viewport
   // If the widget has already been added, it
   // will be adjusted to the new parent's dimensions.
   virtual void add_to_viewport(Vector4 dimensions) = 0;

   // Removed the widget from current viewport.
   virtual void remove_from_viewport() = 0;
};

using IWidgetPtr = std::unique_ptr<IWidget>;

}// namespace triglav::ui_core