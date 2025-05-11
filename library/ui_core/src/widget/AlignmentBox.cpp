#include "widget/AlignmentBox.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_content_offset(const HorizontalAlignment horizontalAlignment, const VerticalAlignment verticalAlignment,
                                 const Vector2 parentSize, const Vector2 contentSize)
{
   return {calculate_alignment(horizontalAlignment, parentSize.x, contentSize.x),
           calculate_alignment(verticalAlignment, parentSize.y, contentSize.y)};
}

}// namespace

AlignmentBox::AlignmentBox(Context& ctx, State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(std::move(state))
{
}

Vector2 AlignmentBox::desired_size(const Vector2 parentSize) const
{
   return parentSize;
}

void AlignmentBox::add_to_viewport(const Vector4 dimensions)
{
   const Vector2 parentSize{dimensions.z, dimensions.w};
   const Vector2 size = m_content->desired_size(parentSize);
   const Vector2 offset = calculate_content_offset(m_state.horizontalAlignment, m_state.verticalAlignment, parentSize, size);
   m_content->add_to_viewport({dimensions.x + offset.x, dimensions.y + offset.y, size.x, size.y});
   m_parentDimensions = dimensions;
}

void AlignmentBox::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void AlignmentBox::on_child_state_changed(IWidget& /*widget*/)
{
   m_content->add_to_viewport(m_parentDimensions);
}

void AlignmentBox::on_event(const Event& event)
{
   const Vector2 size = m_content->desired_size(event.parentSize);
   const Vector2 offset = calculate_content_offset(m_state.horizontalAlignment, m_state.verticalAlignment, event.parentSize, size);
   if (event.mousePosition.x > offset.x && event.mousePosition.x < (offset.x + size.x) && event.mousePosition.y > offset.y &&
       event.mousePosition.y < (offset.y + size.y)) {
      if (event.eventType == Event::Type::MouseMoved && !m_isMouseInside) {
         Event enterEvent;
         enterEvent.eventType = Event::Type::MouseEntered;
         enterEvent.parentSize = size;
         enterEvent.mousePosition = event.mousePosition - offset;
         m_content->on_event(enterEvent);
      }
      m_isMouseInside = true;

      Event subEvent{event};
      subEvent.parentSize = size;
      subEvent.mousePosition -= offset;
      m_content->on_event(subEvent);
   } else if (m_isMouseInside) {
      Event leaveEvent;
      leaveEvent.eventType = Event::Type::MouseLeft;
      leaveEvent.parentSize = size;
      leaveEvent.mousePosition = event.mousePosition - offset;
      m_content->on_event(leaveEvent);
      m_isMouseInside = false;
   }
}

}// namespace triglav::ui_core
