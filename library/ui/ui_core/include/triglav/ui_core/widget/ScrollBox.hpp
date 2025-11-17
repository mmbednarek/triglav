#pragma once

#include "../IWidget.hpp"

#include <optional>

namespace triglav::ui_core {

class ScrollBox final : public ContainerWidget
{
 public:
   struct State
   {
      float offset = 0.0f;
      std::optional<float> max_height;
   };

   ScrollBox(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;

   void on_event(const Event& event) override;

 private:
   State m_state;
   mutable Vector2 m_content_size{};
   mutable Vector2 m_parent_size{};
   mutable float m_height{};
   Vector4 m_stored_dims{};
   Vector4 m_cropping_mask{};
};

}// namespace triglav::ui_core
