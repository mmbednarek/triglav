#include "widget/VerticalLayout.hpp"

namespace triglav::ui_core {

VerticalLayout::VerticalLayout(Context& context, State state, IWidget* parent) :
    LayoutWidget(context, parent),
    m_state(std::move(state))
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

   float height = innerDimensions.w;
   float y{0.0f};
   for (const auto& child : m_children) {
      const auto size = child->desired_size({dimensions.z, height});
      child->add_to_viewport({innerDimensions.x, innerDimensions.y + y, innerDimensions.z, size.y});
      y += size.y + m_state.separation;
      height -= size.y + m_state.separation;
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

void VerticalLayout::on_event(const Event& event)
{
   if (event.eventType == Event::Type::MouseLeft) {
      for (const auto& child : m_children) {
         child->on_event(event);
      }
      return;
   }

   float height = event.parentSize.y;
   float y{m_state.padding.y};

   for (const auto& child : m_children) {
      const auto size = child->desired_size({event.parentSize.x, height});
      if (event.mousePosition.y >= y && event.mousePosition.y < (y + size.y)) {
         Event subEvent{event};
         subEvent.parentSize = size;
         subEvent.mousePosition -= Vector2{m_state.padding.x, y};
         child->on_event(subEvent);

         if (event.eventType == Event::Type::MouseMoved) {
            this->handle_mouse_leave(subEvent, child.get());
         }
         return;
      }

      y += size.y + m_state.separation;
      height -= size.y + m_state.separation;
   }

   if (event.eventType == Event::Type::MouseMoved && m_lastActiveWidget != nullptr) {
      Event leaveEvent{event};
      leaveEvent.eventType = Event::Type::MouseLeft;
      // leaveEvent.mousePosition -= m_lastActiveWidgetOffset;
      m_lastActiveWidget->on_event(leaveEvent);
      m_lastActiveWidget = nullptr;
   }
}

void VerticalLayout::handle_mouse_leave(const Event& event, IWidget* widget)
{
   // FIXME: Leave events have invalid position
   if (m_lastActiveWidget != widget) {
      Event enterEvent{event};
      enterEvent.eventType = Event::Type::MouseEntered;
      widget->on_event(enterEvent);
      if (m_lastActiveWidget != nullptr) {
         Event leaveEvent{event};
         leaveEvent.eventType = Event::Type::MouseLeft;
         m_lastActiveWidget->on_event(leaveEvent);
      }
      m_lastActiveWidget = widget;
   }
}

}// namespace triglav::ui_core
