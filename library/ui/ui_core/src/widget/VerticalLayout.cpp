#include "widget/VerticalLayout.hpp"

namespace triglav::ui_core {

VerticalLayout::VerticalLayout(Context& context, State state, IWidget* parent) :
    LayoutWidget(context, parent),
    m_state(std::move(state))
{
}

Vector2 VerticalLayout::desired_size(const Vector2 available_size) const
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
         result.y += m_state.separation;
         inner_size.y -= m_state.separation;
      }

      const auto child_desired_size = child->desired_size(inner_size);
      if (child_desired_size.x > result.x) {
         result.x = child_desired_size.x;
      }
      result.y += child_desired_size.y;
      inner_size.y -= child_desired_size.y;
   }

   return min_size + result;
}

void VerticalLayout::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   const Vector4 inner_dimensions{dimensions.x + m_state.padding.x, dimensions.y + m_state.padding.y,
                                  dimensions.z - m_state.padding.x - m_state.padding.z,
                                  dimensions.w - m_state.padding.y - m_state.padding.w};

   float height = inner_dimensions.w;
   float y{0.0f};
   for (const auto& child : m_children) {
      const auto size = child->desired_size({dimensions.z, height});
      child->add_to_viewport({inner_dimensions.x, inner_dimensions.y + y, inner_dimensions.z, size.y}, cropping_mask);
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
   if (event.event_type == Event::Type::MouseLeft) {
      for (const auto& child : m_children) {
         child->on_event(event);
      }
      return;
   }

   float height = event.widget_size.y;
   float y{m_state.padding.y};

   for (const auto& child : m_children) {
      const auto size = child->desired_size({event.widget_size.x, height});
      if (event.mouse_position.y >= y && event.mouse_position.y < (y + size.y)) {
         Event sub_event{event};
         sub_event.widget_size = size;
         sub_event.mouse_position -= Vector2{m_state.padding.x, y};
         child->on_event(sub_event);

         if (event.event_type == Event::Type::MouseMoved) {
            this->handle_mouse_leave(sub_event, child.get());
         }
         return;
      }

      y += size.y + m_state.separation;
      height -= size.y + m_state.separation;
   }

   if (event.event_type == Event::Type::MouseMoved && m_last_active_widget != nullptr) {
      Event leave_event{event};
      leave_event.event_type = Event::Type::MouseLeft;
      // leave_event.mouse_position -= m_last_active_widget_offset;
      m_last_active_widget->on_event(leave_event);
      m_last_active_widget = nullptr;
   }
}

void VerticalLayout::handle_mouse_leave(const Event& event, IWidget* widget)
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
