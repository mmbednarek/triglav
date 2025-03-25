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

   AlignmentBox(Context& ctx, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   void on_child_state_changed(IWidget& widget) override;

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

   void on_mouse_click(desktop::MouseButton mouseButton, Vector2 parentSize, Vector2 position) override;

 private:
   Context& m_context;
   State m_state;
   IWidget* m_parent;
   IWidgetPtr m_content;
   Vector4 m_parentDimensions{};
};

}// namespace triglav::ui_core