#pragma once

#include "../IWidget.hpp"

#include "triglav/event/Delegate.hpp"

namespace triglav::ui_core {

class Context;

class Button final : public ContainerWidget
{
 public:
   using Self = Button;
   TG_EVENT(OnClick, desktop::MouseButton)
   TG_EVENT(OnEnter)
   TG_EVENT(OnLeave)

   struct State
   {};

   Button(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;
   void on_event(const Event& event) override;
};

}// namespace triglav::ui_core
