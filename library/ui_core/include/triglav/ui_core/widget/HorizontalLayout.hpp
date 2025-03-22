#pragma once

#include "../IWidget.hpp"

#include <vector>

namespace triglav::ui_core {

class Context;

class HorizontalLayout final : public IWidget
{
 public:
   struct State
   {
      Vector4 padding;
      float separation;
   };

   HorizontalLayout(Context& context, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;
   void on_child_state_changed(IWidget& widget) override;

   IWidget& add_child(IWidgetPtr&& widget);

   template<typename TChild, typename... TArgs>
   TChild& emplace_child(TArgs&&... args)
   {
      return dynamic_cast<TChild&>(this->add_child(std::make_unique<TChild>(std::forward<TArgs>(args)...)));
   }

   template<ConstructableWidget TChild>
   TChild& create_child(typename TChild::State&& state)
   {
      return dynamic_cast<TChild&>(this->add_child(std::make_unique<TChild>(m_context, std::forward<typename TChild::State>(state), this)));
   }

 private:
   Context& m_context;
   State m_state;
   IWidget* m_parent;
   std::vector<IWidgetPtr> m_children;
};

}// namespace triglav::ui_core
