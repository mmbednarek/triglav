#include "widget/Button.hpp"

namespace triglav::ui_core {

Button::Button(Context& ctx, State /*state*/, IWidget* parent) :
    WrappedWidget(ctx),
    m_parent(parent)
{
}

Vector2 Button::desired_size(const Vector2 parentSize) const
{
   return m_content->desired_size(parentSize);
}

void Button::add_to_viewport(const Vector4 dimensions)
{
   m_content->add_to_viewport(dimensions);
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

void Button::on_mouse_click(desktop::MouseButton button, Vector2 /*parentSize*/, Vector2 /*position*/)
{
   event_OnClick.publish(button);
   // Should event be passed along??
}

}// namespace triglav::ui_core