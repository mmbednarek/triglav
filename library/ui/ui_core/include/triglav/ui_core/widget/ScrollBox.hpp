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
      std::optional<float> maxHeight;
   };

   ScrollBox(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;

   void on_event(const Event& event) override;

 private:
   State m_state;
   mutable Vector2 m_contentSize{};
   mutable Vector2 m_parentSize{};
   mutable float m_height{};
   Vector4 m_storedDims{};
   Vector4 m_croppingMask{};
};

}// namespace triglav::ui_core
