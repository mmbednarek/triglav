#pragma once

#include "../IWidget.hpp"

#include "triglav/Math.hpp"

namespace triglav::ui_core {

class Context;

enum class Alignment
{
   TopLeft,
   TopCenter,
   TopRight,
   Left,
   Center,
   Right,
   BottomLeft,
   BottomCenter,
   BottomRight,
};

class AlignmentBox final : public IWidget
{
 public:
   struct State
   {
      Alignment alignment;
   };

   AlignmentBox(Context& ctx, State state);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   IWidget& set_content(IWidgetPtr&& content);

   template<typename T, typename... TArgs>
   T& emplace_content(TArgs&&... args)
   {
      return dynamic_cast<T&>(this->set_content(std::make_unique<T>(std::forward<TArgs>(args)...)));
   }

 private:
   Context& m_context;
   State m_state;
   IWidgetPtr m_content;
};

}// namespace triglav::ui_core