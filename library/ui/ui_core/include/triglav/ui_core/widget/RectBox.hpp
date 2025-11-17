#pragma once

#include "../IWidget.hpp"
#include "../Primitives.hpp"

#include <optional>

namespace triglav::ui_core {

class Context;

class RectBox final : public ContainerWidget
{
 public:
   struct State
   {
      Color color;
      Vector4 border_radius;
      Color border_color;
      float border_width;
   };

   RectBox(Context& context, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void set_color(Vector4 color);

   void on_child_state_changed(IWidget& widget) override;
   void on_event(const Event& event) override;

 private:
   State m_state{};
   RectId m_rect_name{};
   mutable std::optional<Vector2> m_cached_parent_size{};
   mutable Vector2 m_cached_size{};
};

}// namespace triglav::ui_core