#include "DesktopContainerWidget.hpp"

#include "DesktopUI.hpp"

namespace triglav::desktop_ui {

DesktopContainerWidget::DesktopContainerWidget(ui_core::Context& context, IWidget* parent) :
    ui_core::ContainerWidget(context, parent)
{
}

DesktopContext& DesktopContainerWidget::desktop_context()
{
   return dynamic_cast<DesktopContext&>(m_context);
}

const DesktopContext& DesktopContainerWidget::desktop_context() const
{
   return dynamic_cast<const DesktopContext&>(m_context);
}

DesktopProxyWidget::DesktopProxyWidget(ui_core::Context& context, IWidget* parent) :
    ui_core::ProxyWidget(context, parent)
{
}

DesktopContext& DesktopProxyWidget::desktop_context()
{
   return dynamic_cast<DesktopContext&>(m_context);
}

const DesktopContext& DesktopProxyWidget::desktop_context() const
{
   return dynamic_cast<const DesktopContext&>(m_context);
}

}// namespace triglav::desktop_ui
