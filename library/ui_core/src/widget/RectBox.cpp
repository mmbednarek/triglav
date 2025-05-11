#include "widget/RectBox.hpp"
#include "Context.hpp"

#include <Viewport.hpp>

namespace triglav::ui_core {

RectBox::RectBox(Context& context, State state, IWidget* parent) :
    ContainerWidget(context, parent),
    m_state(std::move(state))
{
}

Vector2 RectBox::desired_size(const Vector2 parentSize) const
{
   if (m_cachedParentSize.has_value() && *m_cachedParentSize == parentSize)
      return m_cachedSize;

   m_cachedParentSize.emplace(parentSize);
   m_cachedSize = m_content->desired_size(parentSize);
   return m_cachedSize;
}

void RectBox::add_to_viewport(const Vector4 dimensions)
{
   const Vector4 rectSize{dimensions.x, dimensions.y, dimensions.x + dimensions.z, dimensions.y + dimensions.w};

   if (m_rectName != 0) {
      m_context.viewport().set_rectangle_dims(m_rectName, rectSize);
   } else {
      m_rectName = m_context.viewport().add_rectangle(Rectangle{
         .rect = rectSize,
         .color = m_state.color,
      });
   }

   m_content->add_to_viewport(dimensions);
}

void RectBox::remove_from_viewport()
{
   m_content->remove_from_viewport();
   m_context.viewport().remove_rectangle(m_rectName);
}

void RectBox::set_color(Vector4 color)
{
   if (m_state.color == color)
      return;
   if (m_rectName == 0)
      return;

   m_state.color = color;
   m_context.viewport().set_rectangle_color(m_rectName, color);
}

void RectBox::on_child_state_changed(IWidget& widget)
{
   m_cachedParentSize.reset();
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

void RectBox::on_event(const Event& event)
{
   m_content->on_event(event);
}

}// namespace triglav::ui_core
