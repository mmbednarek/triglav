#include "widget/HorizontalLayout.hpp"

namespace triglav::ui_core {

HorizontalLayout::HorizontalLayout(Context& context, State state, IWidget* parent) :
    LayoutWidget(context, parent),
    m_context(context),
    m_state(std::move(state))
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

void HorizontalLayout::on_event(const Event& event)
{
   if (event.eventType == Event::Type::MouseLeft) {
      for (const auto& child : m_children) {
         child->on_event(event);
      }
      return;
   }

   float width = event.parentSize.x;
   float x{m_state.padding.x};

   for (const auto& child : m_children) {
      const auto size = child->desired_size({width, event.parentSize.y});
      if (event.mousePosition.x >= x && event.mousePosition.x < (x + size.x)) {
         Event subEvent{event};
         subEvent.parentSize = size;
         subEvent.mousePosition -= Vector2{x, m_state.padding.y};
         child->on_event(subEvent);

         if (event.eventType == Event::Type::MouseMoved) {
            this->handle_mouse_leave(subEvent, child.get());
         }
         return;
      }

      x += size.x + m_state.separation;
      width -= size.x + m_state.separation;
   }

   if (event.eventType == Event::Type::MouseMoved && m_lastActiveWidget != nullptr) {
      Event leaveEvent{event};
      leaveEvent.eventType = Event::Type::MouseLeft;
      m_lastActiveWidget->on_event(leaveEvent);
      m_lastActiveWidget = nullptr;
   }
}

void HorizontalLayout::handle_mouse_leave(const Event& event, IWidget* widget)
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
