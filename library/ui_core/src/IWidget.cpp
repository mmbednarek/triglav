#include "IWidget.hpp"

namespace triglav::ui_core {

bool EventVisitor::visit_event(const Event& event)
{
   switch (event.eventType) {
   case Event::Type::MousePressed:
      return this->on_mouse_pressed(event, std::get<Event::Mouse>(event.data));
   case Event::Type::MouseReleased:
      return this->on_mouse_released(event, std::get<Event::Mouse>(event.data));
   case Event::Type::MouseMoved:
      return this->on_mouse_moved(event);
   case Event::Type::MouseEntered:
      return this->on_mouse_entered(event);
   case Event::Type::MouseLeft:
      return this->on_mouse_left(event);
   case Event::Type::MouseScrolled:
      return this->on_mouse_scrolled(event, std::get<Event::Scroll>(event.data));
   case Event::Type::KeyPressed:
      return this->on_key_pressed(event, std::get<Event::Keyboard>(event.data));
   case Event::Type::TextInput:
      return this->on_text_input(event, std::get<Event::TextInput>(event.data));
   }
   return true;
}

BaseWidget::BaseWidget(IWidget* parent) :
    m_parent(parent)
{
}

void BaseWidget::on_child_state_changed(IWidget& widget)
{
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

ContainerWidget::ContainerWidget(Context& context, IWidget* parent) :
    BaseWidget(parent),
    m_context(context)
{
}

IWidget& ContainerWidget::set_content(IWidgetPtr&& content)
{
   m_content = std::move(content);
   return *m_content;
}

LayoutWidget::LayoutWidget(Context& context, IWidget* parent) :
    BaseWidget(parent),
    m_context(context)
{
}

IWidget& LayoutWidget::add_child(IWidgetPtr&& widget)
{
   return *m_children.emplace_back(std::move(widget));
}

}// namespace triglav::ui_core