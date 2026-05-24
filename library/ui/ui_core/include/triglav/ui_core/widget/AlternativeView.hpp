#pragma once

#include "../IWidget.hpp"
#include "../UICore.hpp"

#include "triglav/Math.hpp"

namespace triglav::ui_core {

class Context;

class AlternativeView final : public LayoutWidget
{
 public:
   struct State
   {
      u32 visible_view = 0;
   };

   AlternativeView(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;

   void on_event(const Event& event) override;

   [[nodiscard]] IWidget& current_widget() const;
   void set_visible_widget(u32 index);

 private:
   State m_state;
   bool m_is_added_to_viewport = false;
   Vector4 m_dimensions{};
   Vector4 m_cropping_mask{};
};

}// namespace triglav::ui_core
