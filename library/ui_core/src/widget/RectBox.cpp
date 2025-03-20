#include "widget/RectBox.hpp"
#include "Context.hpp"

#include <Viewport.hpp>

namespace triglav::ui_core {

RectBox::RectBox(Context& context, State state) :
    m_context(context),
    m_state(std::move(state))
{
}

Vector2 RectBox::desired_size(const Vector2 parentSize) const
{
   if (m_cachedParentSize == parentSize)
      return m_cachedSize;

   m_cachedParentSize = parentSize;
   m_cachedSize = m_content->desired_size(parentSize);
   return m_cachedSize;
}

void RectBox::add_to_viewport(const Vector4 dimensions)
{
   m_rectName = m_context.viewport().add_rectangle(Rectangle{
      .rect = {dimensions.x, dimensions.y, dimensions.x + dimensions.z, dimensions.y + dimensions.w},
      .color = m_state.color,
   });
   m_content->add_to_viewport(dimensions);
}

void RectBox::remove_from_viewport()
{
   m_content->remove_from_viewport();
   m_context.viewport().remove_rectangle(m_rectName);
}

IWidget& RectBox::set_content(IWidgetPtr&& content)
{
   m_content = std::move(content);
   return *m_content;
}

}// namespace triglav::ui_core
