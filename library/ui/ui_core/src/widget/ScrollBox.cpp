#include "widget/ScrollBox.hpp"

namespace triglav::ui_core {

constexpr auto g_scroll_factor = 6.0f;

ScrollBox::ScrollBox(Context& ctx, State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(std::move(state))
{
}

Vector2 ScrollBox::desired_size(const Vector2 parent_size) const
{
   m_content_size = this->m_content->desired_size(parent_size);
   m_parent_size = parent_size;
   m_height = m_state.max_height.has_value() ? std::min(*m_state.max_height, parent_size.y) : parent_size.y;
   return {m_content_size.x, m_height};
}

void ScrollBox::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_stored_dims = dimensions;
   m_cropping_mask = min_area(cropping_mask, dimensions);
   this->m_content->add_to_viewport({dimensions.x, dimensions.y + m_state.offset, dimensions.z, dimensions.w}, m_cropping_mask);
}

void ScrollBox::remove_from_viewport()
{
   this->m_content->remove_from_viewport();
}

void ScrollBox::on_event(const Event& event)
{
   if (event.event_type == Event::Type::MouseScrolled) {
      if (m_height > m_content_size.y)
         return;

      auto& data = std::get<Event::Scroll>(event.data);
      const auto new_offset = m_state.offset - g_scroll_factor * data.amount;
      m_state.offset = std::clamp(new_offset, m_height - m_content_size.y, 0.0f);
      this->m_content->add_to_viewport({m_stored_dims.x, m_stored_dims.y + m_state.offset, m_stored_dims.z, m_stored_dims.w},
                                       m_cropping_mask);
   } else if (event.mouse_position.y >= m_state.offset && event.mouse_position.y < (m_state.offset + m_content_size.y)) {
      Event sub_event{event};
      sub_event.mouse_position.y -= m_state.offset;
      sub_event.parent_size = {m_content_size.x, m_height};
      this->m_content->on_event(sub_event);
   }
}
}// namespace triglav::ui_core