#include "widget/Padding.hpp"

namespace triglav::ui_core {

Padding::Padding(Context& ctx, State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_padding(state)
{
}

Vector2 Padding::desired_size(Vector2 parent_size) const
{
   const auto child_size = m_content->desired_size({parent_size.x - m_padding.x - m_padding.z, parent_size.y - m_padding.y - m_padding.w});
   return {child_size.x + m_padding.x + m_padding.z, child_size.y + m_padding.y + m_padding.w};
}

void Padding::add_to_viewport(Vector4 dimensions, Vector4 cropping_mask)
{
   m_content->add_to_viewport({dimensions.x + m_padding.x, dimensions.y + m_padding.y, dimensions.z - m_padding.x - m_padding.z,
                               dimensions.w - m_padding.y - m_padding.w},
                              cropping_mask);
   m_dimensions = dimensions;
}

void Padding::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void Padding::on_event(const Event& event)
{
   if (event.mouse_position.x >= m_padding.x && event.mouse_position.y >= m_padding.y &&
       event.mouse_position.x < m_dimensions.z - m_padding.z && event.mouse_position.y < m_dimensions.w - m_padding.w) {
      Event sub_event = event;
      sub_event.mouse_position -= Vector2{m_padding.x, m_padding.y};
      m_content->on_event(sub_event);
   }
}

}// namespace triglav::ui_core
