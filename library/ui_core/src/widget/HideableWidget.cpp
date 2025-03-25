#include "widget/HideableWidget.hpp"

namespace triglav::ui_core {

HideableWidget::HideableWidget(Context& ctx, const State state, IWidget* parent) :
    WrappedWidget(ctx),
    m_state(state),
    m_parent(parent)
{
}

Vector2 HideableWidget::desired_size(const Vector2 parentSize) const
{
   if (m_state.isHidden) {
      return {0, 0};
   }
   return m_content->desired_size(parentSize);
}

void HideableWidget::add_to_viewport(const Vector4 dimensions)
{
   m_parentDimensions = dimensions;

   if (m_state.isHidden)
      return;
   m_content->add_to_viewport(dimensions);
}

void HideableWidget::remove_from_viewport()
{
   if (m_state.isHidden)
      return;
   m_content->remove_from_viewport();
}

void HideableWidget::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

void HideableWidget::on_mouse_click(const desktop::MouseButton button, const Vector2 parentSize, const Vector2 position)
{
   if (m_state.isHidden)
      return;

   m_content->on_mouse_click(button, parentSize, position);
}

void HideableWidget::set_is_hidden(const bool value)
{
   if (value) {
      this->remove_from_viewport();
   } else {
      this->add_to_viewport(m_parentDimensions);
   }
   m_state.isHidden = value;

   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(*this);
   }
}

const HideableWidget::State& HideableWidget::state() const
{
   return m_state;
}

}// namespace triglav::ui_core