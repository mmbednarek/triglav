#include "IWidget.hpp"

namespace triglav::ui_core {

WrappedWidget::WrappedWidget(Context& context) :
    m_context(context)
{
}

IWidget& WrappedWidget::set_content(IWidgetPtr&& content)
{
   m_content = std::move(content);
   return *m_content;
}

}// namespace triglav::ui_core