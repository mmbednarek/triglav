#pragma once

#include "../IWidget.hpp"

#include "triglav/Name.hpp"

namespace triglav::ui_core {

class Context;

class RectBox final : public IWidget
{
 public:
   struct State
   {
      Vector4 color;
   };

   RectBox(Context& context, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   IWidget& set_content(IWidgetPtr&& content);

   template<typename T, typename... TArgs>
   T& emplace_content(TArgs&&... args)
   {
      return dynamic_cast<T&>(this->set_content(std::make_unique<T>(std::forward<TArgs>(args)...)));
   }

   template<ConstructableWidget T>
   T& create_content(typename T::State&& state)
   {
      return dynamic_cast<T&>(this->set_content(std::make_unique<T>(m_context, std::forward<typename T::State>(state), this)));
   }

   void on_child_state_changed(IWidget& widget) override;
   void on_mouse_click(desktop::MouseButton mouseButton, Vector2 parentSize, Vector2 position) override;

 private:
   Context& m_context;
   State m_state{};
   IWidget* m_parent;
   IWidgetPtr m_content;
   Name m_rectName{};
   mutable std::optional<Vector2> m_cachedParentSize{};
   mutable Vector2 m_cachedSize{};
};

}// namespace triglav::ui_core