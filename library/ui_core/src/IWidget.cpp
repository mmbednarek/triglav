#include "IWidget.hpp"

namespace triglav::ui_core {

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