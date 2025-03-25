#pragma once

#include "../IWidget.hpp"

namespace triglav::ui_core {

class Context;

class HideableWidget final : public WrappedWidget
{
 public:
   struct State
   {
      bool isHidden = false;
   };

   HideableWidget(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;
   void on_mouse_click(desktop::MouseButton button, Vector2 parentSize, Vector2 position) override;

   void set_is_hidden(bool value);
   [[nodiscard]] const State& state() const;


 private:
   State m_state;
   IWidget* m_parent;
   Vector4 m_parentDimensions{};
};

}// namespace triglav::ui_core
