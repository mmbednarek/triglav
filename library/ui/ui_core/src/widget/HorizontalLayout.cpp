#include "widget/HorizontalLayout.hpp"

namespace triglav::ui_core {

HorizontalLayout::HorizontalLayout(Context& context, State state, IWidget* parent) :
    LayoutWidget(context, parent),
    m_state(std::move(state))
{
}

Vector2 HorizontalLayout::desired_size(const Vector2 available_size) const
{
   const Vector2 min_size{m_state.padding.x + m_state.padding.z, m_state.padding.y + m_state.padding.w};

   if (m_children.empty()) {
      return min_size;
   }

   Vector2 inner_size = available_size - min_size;
   Vector2 result{};
   bool is_first = true;
   for (const auto& child : m_children) {
      if (is_first) {
         is_first = false;
      } else {
         result.x += m_state.separation;
         inner_size.x -= m_state.separation;
      }

      const auto child_desired_size = child->desired_size(inner_size);
      result.x += child_desired_size.x;
      if (child_desired_size.y > result.y) {
         result.y = child_desired_size.y;
      }
      inner_size.x -= child_desired_size.x;
   }

   return min_size + result;
}

namespace {

float initial_position(const HorizontalAlignment alignment, const float container_width, const float content_width)
{
   switch (alignment) {
   case HorizontalAlignment::Left:
      return 0.0f;
   case HorizontalAlignment::Center:
      return 0.5f * (container_width - content_width);
   case HorizontalAlignment::Right:
      return container_width - content_width;
   }
   return 0.0f;
}

}// namespace

void HorizontalLayout::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   const Vector4 inner_dimensions{dimensions.x + m_state.padding.x, dimensions.y + m_state.padding.y,
                                  dimensions.z - m_state.padding.x - m_state.padding.z,
                                  dimensions.w - m_state.padding.y - m_state.padding.w};

   const auto content_size = this->desired_size(dimensions);
   float width = inner_dimensions.z;
   float x = initial_position(m_state.gravity, width, content_size.x);
   for (const auto& child : m_children) {
      const auto size = child->desired_size({width, dimensions.w});
      child->add_to_viewport({inner_dimensions.x + x, inner_dimensions.y, size.x, inner_dimensions.w}, cropping_mask);
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
   if (event.event_type == Event::Type::MouseLeft) {
      for (const auto& child : m_children) {
         child->on_event(event);
      }
      return;
   }

   float width = event.widget_size.x;
   float x{m_state.padding.x};

   for (const auto& child : m_children) {
      const auto size = child->desired_size({width, event.widget_size.y});
      if (event.mouse_position.x >= x && event.mouse_position.x < (x + size.x)) {
         Event sub_event{event};
         sub_event.widget_size = size;
         sub_event.mouse_position -= Vector2{x, m_state.padding.y};
         child->on_event(sub_event);

         if (event.event_type == Event::Type::MouseMoved) {
            this->handle_mouse_leave(sub_event, child.get());
         }
         return;
      }

      x += size.x + m_state.separation;
      width -= size.x + m_state.separation;
   }

   if (event.event_type == Event::Type::MouseMoved && m_last_active_widget != nullptr) {
      Event leave_event{event};
      leave_event.event_type = Event::Type::MouseLeft;
      m_last_active_widget->on_event(leave_event);
      m_last_active_widget = nullptr;
   }
}

void HorizontalLayout::handle_mouse_leave(const Event& event, IWidget* widget)
{
   // FIXME: Leave events have invalid position
   if (m_last_active_widget != widget) {
      Event enter_event{event};
      enter_event.event_type = Event::Type::MouseEntered;
      widget->on_event(enter_event);
      if (m_last_active_widget != nullptr) {
         Event leave_event{event};
         leave_event.event_type = Event::Type::MouseLeft;
         m_last_active_widget->on_event(leave_event);
      }
      m_last_active_widget = widget;
   }
}

}// namespace triglav::ui_core
