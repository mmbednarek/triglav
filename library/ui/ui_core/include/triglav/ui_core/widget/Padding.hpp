#pragma once

#include "../IWidget.hpp"
#include "../UICore.hpp"

namespace triglav::ui_core {

class Context;

class Padding final : public ContainerWidget
{
 public:
   using State = Vector4;

   Padding(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;

   void on_event(const Event& event) override;

 private:
   Vector4 m_padding;
   Vector4 m_dimensions{};
};

}// namespace triglav::ui_core
