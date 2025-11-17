#include "widget/AlignmentBox.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_content_offset(const std::optional<HorizontalAlignment> horizontal_alignment,
                                 const std::optional<VerticalAlignment> vertical_alignment, const Vector2 parent_size,
                                 const Vector2 content_size)
{
   Vector2 result{0, 0};
   if (horizontal_alignment.has_value()) {
      result.x = calculate_alignment(*horizontal_alignment, parent_size.x, content_size.x);
   }
   if (vertical_alignment.has_value()) {
      result.y = calculate_alignment(*vertical_alignment, parent_size.y, content_size.y);
   }
   return result;
}

}// namespace

AlignmentBox::AlignmentBox(Context& ctx, State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(std::move(state))
{
}

Vector2 AlignmentBox::desired_size(const Vector2 parent_size) const
{
   if (m_state.horizontal_alignment.has_value() && m_state.vertical_alignment.has_value())
      return parent_size;

   auto result = m_content->desired_size(parent_size);
   if (m_state.horizontal_alignment.has_value()) {
      result.x = parent_size.x;
   }
   if (m_state.vertical_alignment.has_value()) {
      result.y = parent_size.y;
   }

   return result;
}

void AlignmentBox::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   const Vector2 parent_size = rect_size(dimensions);
   const Vector2 size = m_content->desired_size(parent_size);
   const Vector2 offset = calculate_content_offset(m_state.horizontal_alignment, m_state.vertical_alignment, parent_size, size);
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
   const Vector2 size = m_content->desired_size(event.parent_size);
   const Vector2 offset = calculate_content_offset(m_state.horizontal_alignment, m_state.vertical_alignment, event.parent_size, size);
   if (event.mouse_position.x > offset.x && event.mouse_position.x < (offset.x + size.x) && event.mouse_position.y > offset.y &&
       event.mouse_position.y < (offset.y + size.y)) {
      if (event.event_type == Event::Type::MouseMoved && !m_is_mouse_inside) {
         Event enter_event;
         enter_event.event_type = Event::Type::MouseEntered;
         enter_event.parent_size = size;
         enter_event.mouse_position = event.mouse_position - offset;
         enter_event.global_mouse_position = event.mouse_position;
         m_content->on_event(enter_event);
      }
      m_is_mouse_inside = true;

      Event sub_event{event};
      sub_event.parent_size = size;
      sub_event.mouse_position -= offset;
      m_content->on_event(sub_event);
   } else if (m_is_mouse_inside) {
      Event leave_event;
      leave_event.event_type = Event::Type::MouseLeft;
      leave_event.parent_size = size;
      leave_event.mouse_position = event.mouse_position - offset;
      leave_event.global_mouse_position = event.mouse_position;
      m_content->on_event(leave_event);
      m_is_mouse_inside = false;
   }
}

}// namespace triglav::ui_core
