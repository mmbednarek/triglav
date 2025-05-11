#pragma once

#include "../IWidget.hpp"
#include "../UICore.hpp"

#include "triglav/Math.hpp"

namespace triglav::ui_core {

class Context;

class AlignmentBox final : public ContainerWidget
{
 public:
   struct State
   {
      HorizontalAlignment horizontalAlignment;
      VerticalAlignment verticalAlignment;
   };

   AlignmentBox(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;

   void on_event(const Event& event) override;

 private:
   State m_state;
   Vector4 m_parentDimensions{};
   bool m_isMouseInside = false;
};

}// namespace triglav::ui_core