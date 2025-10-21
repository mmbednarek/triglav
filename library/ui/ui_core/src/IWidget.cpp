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

ProxyWidget::ProxyWidget(Context& context, IWidget* parent) :
    ContainerWidget(context, parent)
{
}

Vector2 ProxyWidget::desired_size(const Vector2 parentSize) const
{
   return m_content->desired_size(parentSize);
}

void ProxyWidget::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_content->add_to_viewport(dimensions, croppingMask);
}

void ProxyWidget::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void ProxyWidget::on_event(const Event& event)
{
   m_content->on_event(event);
}

}// namespace triglav::ui_core