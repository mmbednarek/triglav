#include "widget/HorizontalLayout.hpp"

namespace triglav::ui_core {

HorizontalLayout::HorizontalLayout(Context& context, State state, IWidget* parent) :
    m_context(context),
    m_state(std::move(state)),
    m_parent(parent)
{
}

Vector2 HorizontalLayout::desired_size(const Vector2 parentSize) const
{
   const Vector2 minSize{m_state.padding.x + m_state.padding.z, m_state.padding.y + m_state.padding.w};

   if (m_children.empty()) {
      return minSize;
   }

   Vector2 innerSize = parentSize - minSize;
   Vector2 result{};
   bool isFirst = true;
   for (const auto& child : m_children) {
      if (isFirst) {
         isFirst = false;
      } else {
         result.x += m_state.separation;
         innerSize.x -= m_state.separation;
      }

      const auto childDesiredSize = child->desired_size(innerSize);
      result.x += childDesiredSize.x;
      if (childDesiredSize.y > result.y) {
         result.y = childDesiredSize.y;
      }
      innerSize.x -= childDesiredSize.x;
   }

   return minSize + result;
}

void HorizontalLayout::add_to_viewport(const Vector4 dimensions)
{
   const Vector4 innerDimensions{dimensions.x + m_state.padding.x, dimensions.y + m_state.padding.y,
                                 dimensions.z - m_state.padding.x - m_state.padding.z,
                                 dimensions.w - m_state.padding.y - m_state.padding.w};

   float width = innerDimensions.z;
   float x{0.0f};
   for (const auto& child : m_children) {
      const auto size = child->desired_size({width, dimensions.w});
      child->add_to_viewport({innerDimensions.x + x, innerDimensions.y, size.x, innerDimensions.w});
      x += size.x + m_state.separation;
      width -= size.x + m_state.separation;
   }
}

void HorizontalLayout::remove_from_viewport()
{
   for (const auto& child : m_children) {
      child->remove_from_viewport();
   }
}

void HorizontalLayout::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

void HorizontalLayout::on_mouse_click(const desktop::MouseButton mouseButton, const Vector2 parentSize, const Vector2 position)
{
   float width = parentSize.x;
   float x{m_state.padding.x};

   for (const auto& child : m_children) {
      const auto size = child->desired_size({width, parentSize.y});
      if (position.x >= x && position.x < (x + size.x)) {
         child->on_mouse_click(mouseButton, size, position - Vector2{x, m_state.padding.y});
         return;
      }

      x += size.x + m_state.separation;
      width -= size.x + m_state.separation;
   }
}

IWidget& HorizontalLayout::add_child(IWidgetPtr&& widget)
{
   return *m_children.emplace_back(std::move(widget));
}

}// namespace triglav::ui_core
