#include "widget/VerticalLayout.hpp"

namespace triglav::ui_core {

VerticalLayout::VerticalLayout(Context& context, State state, IWidget* parent) :
    m_context(context),
    m_state(std::move(state)),
    m_parent(parent)
{
}

Vector2 VerticalLayout::desired_size(const Vector2 parentSize) const
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
         result.y += m_state.separation;
         innerSize.y -= m_state.separation;
      }

      const auto childDesiredSize = child->desired_size(innerSize);
      if (childDesiredSize.x > result.x) {
         result.x = childDesiredSize.x;
      }
      result.y += childDesiredSize.y;
      innerSize.y -= childDesiredSize.y;
   }

   return minSize + result;
}

void VerticalLayout::add_to_viewport(const Vector4 dimensions)
{
   const Vector4 innerDimensions{dimensions.x + m_state.padding.x, dimensions.y + m_state.padding.y,
                                 dimensions.z - m_state.padding.x - m_state.padding.z,
                                 dimensions.w - m_state.padding.y - m_state.padding.w};

   const auto childCount = static_cast<float>(m_children.size());
   const float height = (innerDimensions.w - m_state.padding.y - m_state.separation * (childCount - 1.0f)) / childCount;
   float y{0.0f};
   for (const auto& child : m_children) {
      const auto size = child->desired_size({dimensions.z, height});
      child->add_to_viewport({innerDimensions.x, innerDimensions.y + y, innerDimensions.z, size.y});
      y += size.y + m_state.separation;
   }
}

void VerticalLayout::remove_from_viewport()
{
   for (const auto& child : m_children) {
      child->remove_from_viewport();
   }
}

void VerticalLayout::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

IWidget& VerticalLayout::add_child(IWidgetPtr&& widget)
{
   return *m_children.emplace_back(std::move(widget));
}

}// namespace triglav::ui_core
