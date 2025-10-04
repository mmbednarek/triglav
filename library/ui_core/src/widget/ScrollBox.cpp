#include "widget/ScrollBox.hpp"

#include <spdlog/spdlog.h>

namespace triglav::ui_core {

constexpr auto g_scrollFactor = 2.0f;

ScrollBox::ScrollBox(Context& ctx, State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(std::move(state))
{
}

Vector2 ScrollBox::desired_size(const Vector2 parentSize) const
{
   m_contentSize = this->m_content->desired_size(parentSize);
   m_parentSize = parentSize;
   m_height = m_state.maxHeight.has_value() ? std::min(*m_state.maxHeight, parentSize.y) : parentSize.y;
   return {m_contentSize.x, m_height};
}

void ScrollBox::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_storedDims = dimensions;
   m_croppingMask = min_area(croppingMask, dimensions);
   this->m_content->add_to_viewport({dimensions.x, dimensions.y + m_state.offset, dimensions.z, dimensions.w}, m_croppingMask);
}

void ScrollBox::remove_from_viewport()
{
   this->m_content->remove_from_viewport();
}

void ScrollBox::on_event(const Event& event)
{
   if (event.eventType == Event::Type::MouseScrolled) {
      if (m_height > m_contentSize.y)
         return;

      auto& data = std::get<Event::Scroll>(event.data);
      const auto newOffset = m_state.offset - g_scrollFactor * data.amount;
      m_state.offset = std::clamp(newOffset, m_height - m_contentSize.y, 0.0f);
      this->m_content->add_to_viewport({m_storedDims.x, m_storedDims.y + m_state.offset, m_storedDims.z, m_storedDims.w}, m_croppingMask);
   } else if (event.mousePosition.y >= m_state.offset && event.mousePosition.y < (m_state.offset + m_contentSize.y)) {
      Event subEvent{event};
      subEvent.mousePosition.y -= m_state.offset;
      subEvent.parentSize = {m_contentSize.x, m_height};
      this->m_content->on_event(subEvent);
   }
}
}// namespace triglav::ui_core