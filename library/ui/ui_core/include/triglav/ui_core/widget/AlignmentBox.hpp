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
      std::optional<HorizontalAlignment> horizontal_alignment;
      std::optional<VerticalAlignment> vertical_alignment;
   };

   AlignmentBox(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;

   void on_event(const Event& event) override;

 private:
   State m_state;
   Vector4 m_parent_dimensions{};
   Vector4 m_cropping_mask{};
   bool m_is_mouse_inside = false;
};

}// namespace triglav::ui_core