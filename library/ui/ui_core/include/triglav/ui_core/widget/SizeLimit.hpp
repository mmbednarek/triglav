#pragma once

#include "../IWidget.hpp"
#include "../UICore.hpp"

#include "triglav/Math.hpp"

namespace triglav::ui_core {

class Context;

class SizeLimit final : public ContainerWidget
{
 public:
   struct State
   {
      Vector2 max_size;
   };

   SizeLimit(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;

   void on_event(const Event& event) override;

 private:
   State m_state;
};

}// namespace triglav::ui_core
