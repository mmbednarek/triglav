#pragma once

#include "../IWidget.hpp"

#include "triglav/event/Delegate.hpp"

namespace triglav::ui_core {

class Context;

class Button final : public WrappedWidget
{
 public:
   using Self = Button;
   TG_EVENT(OnClick, desktop::MouseButton)

   struct State
   {};

   Button(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;
   void on_mouse_click(desktop::MouseButton button, Vector2 parentSize, Vector2 position) override;

 private:
   IWidget* m_parent;
};

}// namespace triglav::ui_core
