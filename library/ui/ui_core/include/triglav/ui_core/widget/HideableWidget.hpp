#pragma once

#include "../IWidget.hpp"

namespace triglav::ui_core {

class Context;

class HideableWidget final : public ContainerWidget
{
 public:
   struct State
   {
      bool isHidden = false;
   };

   HideableWidget(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;
   void on_event(const Event& event) override;

   void set_is_hidden(bool value);
   [[nodiscard]] const State& state() const;


 private:
   State m_state;
   Vector4 m_parentDimensions{};
   Vector4 m_croppingMask{};
};

}// namespace triglav::ui_core
