#pragma once

#include "../IWidget.hpp"

#include "triglav/Math.hpp"

#include <vector>

namespace triglav::ui_core {

class Context;

class VerticalLayout : public IWidget
{
 public:
   struct State
   {
      Vector4 padding;
      float separation;
   };

   VerticalLayout(Context& context, State state);
   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   IWidget& add_child(IWidgetPtr&& widget);

   template<typename TChild, typename... TArgs>
   TChild& emplace_child(TArgs&&... args)
   {
      return dynamic_cast<TChild&>(this->add_child(std::make_unique<TChild>(std::forward<TArgs>(args)...)));
   }

 private:
   Context& m_context;

   State m_state;
   std::vector<IWidgetPtr> m_children;
};

}// namespace triglav::ui_core