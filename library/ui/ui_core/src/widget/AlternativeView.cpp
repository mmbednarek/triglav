#include "widget/AlternativeView.hpp"

namespace triglav::ui_core {

AlternativeView::AlternativeView(Context& ctx, State state, IWidget* parent) :
    LayoutWidget(ctx, parent),
    m_state(state)
{
}

Vector2 AlternativeView::desired_size(const Vector2 available_size) const
{
   return this->current_widget().desired_size(available_size);
}

void AlternativeView::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   this->current_widget().add_to_viewport(dimensions, cropping_mask);
   m_is_added_to_viewport = true;
   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;
}

void AlternativeView::remove_from_viewport()
{
   this->current_widget().remove_from_viewport();
   m_is_added_to_viewport = false;
}

void AlternativeView::on_child_state_changed(IWidget& widget)
{
   LayoutWidget::on_child_state_changed(widget);
}

void AlternativeView::on_event(const Event& event)
{
   this->current_widget().on_event(event);
}

IWidget& AlternativeView::current_widget() const
{
   assert(m_state.visible_view < m_children.size());
   return *m_children.at(m_state.visible_view);
}

void AlternativeView::set_visible_widget(const u32 index)
{
   if (index >= m_children.size())
      return;

   if (m_is_added_to_viewport) {
      this->current_widget().remove_from_viewport();
   }

   m_state.visible_view = index;

   if (m_is_added_to_viewport) {
      this->add_to_viewport(m_dimensions, m_cropping_mask);
   }
}

}// namespace triglav::ui_core
