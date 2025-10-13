#include "widget/Button.hpp"

namespace triglav::ui_core {

Button::Button(Context& ctx, State /*state*/, IWidget* parent) :
    ContainerWidget(ctx, parent)
{
}

Vector2 Button::desired_size(const Vector2 parentSize) const
{
   return m_content->desired_size(parentSize);
}

void Button::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_content->add_to_viewport(dimensions, croppingMask);
}

void Button::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void Button::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

void Button::on_event(const Event& event)
{
   switch (event.eventType) {
   case Event::Type::MouseReleased:
      event_OnClick.publish(std::get<Event::Mouse>(event.data).button);
      break;
   case Event::Type::MouseEntered:
      event_OnEnter.publish();
      break;
   case Event::Type::MouseLeft:
      event_OnLeave.publish();
      break;
   default:
      break;
   }
}

}// namespace triglav::ui_core