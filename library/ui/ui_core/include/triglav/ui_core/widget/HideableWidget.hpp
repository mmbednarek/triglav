#pragma once

#include "../IWidget.hpp"

namespace triglav::ui_core {

class Context;

class HideableWidget final : public ContainerWidget
{
 public:
   struct State
   {
      bool is_hidden = false;
   };

   HideableWidget(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;
   void on_event(const Event& event) override;

   void set_is_hidden(bool value);
   [[nodiscard]] const State& state() const;


 private:
   State m_state;
   Vector4 m_parent_dimensions{};
   Vector4 m_cropping_mask{};
};

}// namespace triglav::ui_core
