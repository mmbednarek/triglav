#include "widget/AlignmentBox.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_content_offset(const std::optional<HorizontalAlignment> horizontal_alignment,
                                 const std::optional<VerticalAlignment> vertical_alignment, const Vector2 available_size,
                                 const Vector2 content_size)
{
   Vector2 result{0, 0};
   if (horizontal_alignment.has_value()) {
      result.x = calculate_alignment(*horizontal_alignment, available_size.x, content_size.x);
   }
   if (vertical_alignment.has_value()) {
      result.y = calculate_alignment(*vertical_alignment, available_size.y, content_size.y);
   }
   return result;
}

Vector2 calculate_content_size(const std::optional<HorizontalAlignment> horizontal_alignment,
                               const std::optional<VerticalAlignment> vertical_alignment, const Vector2 available_size,
                               Vector2 content_size)
{
   if (!horizontal_alignment.has_value()) {
      content_size.x = available_size.x;
   }
   if (!vertical_alignment.has_value()) {
      content_size.y = available_size.y;
   }
   return content_size;
}

}// namespace

AlignmentBox::AlignmentBox(Context& ctx, State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(std::move(state))
{
}

Vector2 AlignmentBox::desired_size(const Vector2 available_size) const
{
   if (m_state.horizontal_alignment.has_value() && m_state.vertical_alignment.has_value())
      return available_size;

   auto result = m_content->desired_size(available_size);
   if (m_state.horizontal_alignment.has_value()) {
      result.x = available_size.x;
   }
   if (m_state.vertical_alignment.has_value()) {
      result.y = available_size.y;
   }

   return result;
}

void AlignmentBox::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   const Vector2 available_size = rect_size(dimensions);
   const Vector2 size = calculate_content_size(m_state.horizontal_alignment, m_state.vertical_alignment, available_size,
                                               m_content->desired_size(available_size));
   const Vector2 offset = calculate_content_offset(m_state.horizontal_alignment, m_state.vertical_alignment, available_size, size);
   m_content->add_to_viewport({dimensions.x + offset.x, dimensions.y + offset.y, size.x, size.y}, cropping_mask);
   m_parent_dimensions = dimensions;
   m_cropping_mask = cropping_mask;
}

void AlignmentBox::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void AlignmentBox::on_child_state_changed(IWidget& /*widget*/)
{
   m_content->add_to_viewport(m_parent_dimensions, m_cropping_mask);
}

void AlignmentBox::on_event(const Event& event)
{
   const Vector2 size = calculate_content_size(m_state.horizontal_alignment, m_state.vertical_alignment, rect_size(m_parent_dimensions),
                                               m_content->desired_size(event.widget_size));
   const Vector2 offset = calculate_content_offset(m_state.horizontal_alignment, m_state.vertical_alignment, event.widget_size, size);
   if (event.mouse_position.x > offset.x && event.mouse_position.x < (offset.x + size.x) && event.mouse_position.y > offset.y &&
       event.mouse_position.y < (offset.y + size.y)) {
      if (event.event_type == Event::Type::MouseMoved && !m_is_mouse_inside) {
         Event enter_event;
         enter_event.event_type = Event::Type::MouseEntered;
         enter_event.widget_size = size;
         enter_event.mouse_position = event.mouse_position - offset;
         enter_event.global_mouse_position = event.mouse_position;
         m_content->on_event(enter_event);
      }
      m_is_mouse_inside = true;

      Event sub_event{event};
      sub_event.widget_size = size;
      sub_event.mouse_position -= offset;
      m_content->on_event(sub_event);
   } else if (m_is_mouse_inside) {
      Event leave_event;
      leave_event.event_type = Event::Type::MouseLeft;
      leave_event.widget_size = size;
      leave_event.mouse_position = event.mouse_position - offset;
      leave_event.global_mouse_position = event.mouse_position;
      m_content->on_event(leave_event);
      m_is_mouse_inside = false;
   }
}

}// namespace triglav::ui_core
