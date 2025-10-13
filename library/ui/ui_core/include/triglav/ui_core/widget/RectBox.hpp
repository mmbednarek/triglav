#pragma once

#include "../IWidget.hpp"
#include "../Primitives.hpp"

#include "triglav/Name.hpp"

namespace triglav::ui_core {

class Context;

class RectBox final : public ContainerWidget
{
 public:
   struct State
   {
      Color color;
      Vector4 borderRadius;
      Color borderColor;
      float borderWidth;
   };

   RectBox(Context& context, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void set_color(Vector4 color);

   void on_child_state_changed(IWidget& widget) override;
   void on_event(const Event& event) override;

 private:
   State m_state{};
   RectId m_rectName{};
   mutable std::optional<Vector2> m_cachedParentSize{};
   mutable Vector2 m_cachedSize{};
};

}// namespace triglav::ui_core