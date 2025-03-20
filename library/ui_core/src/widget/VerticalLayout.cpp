#include "widget/VerticalLayout.hpp"

namespace triglav::ui_core {

VerticalLayout::VerticalLayout(Context& context, State state) :
    m_context(context),
    m_state(std::move(state))
{
}

Vector2 VerticalLayout::desired_size(const Vector2 parentSize) const
{
   const Vector2 minSize{m_state.padding.x + m_state.padding.z, m_state.padding.y + m_state.padding.w};

   if (m_children.empty()) {
      return minSize;
   }

   const Vector2 innerSize = parentSize - minSize;
   const auto childCount = static_cast<float>(m_children.size());
   const Vector2 childSize{innerSize.x, (innerSize.y - m_state.separation * (childCount - 1)) / childCount};

   Vector2 result{};

   bool isFirst = true;
   for (const auto& child : m_children) {
      if (isFirst) {
         isFirst = false;
      } else {
         result.y += m_state.separation;
      }

      const auto childDesiredSize = child->desired_size(childSize);
      if (childDesiredSize.x > result.x) {
         result.x = childDesiredSize.x;
      }
      result.y += childDesiredSize.y;
   }

   return minSize + result;
}

void VerticalLayout::add_to_viewport(const Vector4 dimensions)
{
   const Vector4 innerDimensions{dimensions.x + m_state.padding.x, dimensions.y + m_state.padding.y, dimensions.z - m_state.padding.z,
                                 dimensions.w - m_state.padding.w};

   const auto childCount = static_cast<float>(m_children.size());
   const float height = (innerDimensions.w - m_state.padding.y - m_state.separation * (childCount - 1.0f)) / childCount;
   float y{0.0f};
   for (const auto& child : m_children) {
      child->add_to_viewport({innerDimensions.x, innerDimensions.y + y, innerDimensions.z, innerDimensions.y + height});
      y += height + m_state.separation;
   }
}

void VerticalLayout::remove_from_viewport()
{
   for (const auto& child : m_children) {
      child->remove_from_viewport();
   }
}

IWidget& VerticalLayout::add_child(IWidgetPtr&& widget)
{
   return *m_children.emplace_back(std::move(widget));
}

}// namespace triglav::ui_core
