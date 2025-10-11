#include "IWidget.hpp"

namespace triglav::ui_core {

void EventVisitor::visit_event(const Event& event)
{
   switch (event.eventType) {
   case Event::Type::MousePressed:
      this->on_mouse_pressed(event, std::get<Event::Mouse>(event.data));
      break;
   case Event::Type::MouseReleased:
      this->on_mouse_released(event, std::get<Event::Mouse>(event.data));
      break;
   case Event::Type::MouseMoved:
      this->on_mouse_moved(event);
      break;
   case Event::Type::MouseEntered:
      this->on_mouse_entered(event);
      break;
   case Event::Type::MouseLeft:
      this->on_mouse_left(event);
      break;
   case Event::Type::MouseScrolled:
      this->on_mouse_scrolled(event, std::get<Event::Scroll>(event.data));
      break;
   case Event::Type::KeyPressed:
      this->on_key_pressed(event, std::get<Event::Keyboard>(event.data));
      break;
   case Event::Type::TextInput:
      this->on_text_input(event, std::get<Event::TextInput>(event.data));
      break;
   }
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