#include "widget/HideableWidget.hpp"

namespace triglav::ui_core {

HideableWidget::HideableWidget(Context& ctx, const State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(state)
{
}

Vector2 HideableWidget::desired_size(const Vector2 available_size) const
{
   if (m_state.is_hidden) {
      return {0, 0};
   }
   return m_content->desired_size(available_size);
}

void HideableWidget::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_parent_dimensions = dimensions;
   m_cropping_mask = cropping_mask;

   if (m_state.is_hidden)
      return;
   m_content->add_to_viewport(dimensions, cropping_mask);
}

void HideableWidget::remove_from_viewport()
{
   if (m_state.is_hidden)
      return;
   m_content->remove_from_viewport();
}

void HideableWidget::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

void HideableWidget::on_event(const Event& event)
{
   if (!m_state.is_hidden) {
      m_content->on_event(event);
   }
}

void HideableWidget::set_is_hidden(const bool value)
{
   m_state.is_hidden = value;
   if (m_state.is_hidden) {
      this->remove_from_viewport();
   } else {
      this->add_to_viewport(m_parent_dimensions, m_cropping_mask);
   }

   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(*this);
   }
}

const HideableWidget::State& HideableWidget::state() const
{
   return m_state;
}

}// namespace triglav::ui_core