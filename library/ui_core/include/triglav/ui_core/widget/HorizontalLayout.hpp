#pragma once

#include "../IWidget.hpp"
#include "../UICore.hpp"

namespace triglav::ui_core {

class Context;

class HorizontalLayout final : public LayoutWidget
{
 public:
   struct State
   {
      Vector4 padding;
      float separation;
      HorizontalAlignment gravity = HorizontalAlignment::Left;
   };

   HorizontalLayout(Context& context, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;
   void on_child_state_changed(IWidget& widget) override;
   void on_event(const Event& event) override;

 private:
   void handle_mouse_leave(const Event& event, IWidget* widget);

   State m_state;
   IWidget* m_lastActiveWidget{nullptr};
};

}// namespace triglav::ui_core
