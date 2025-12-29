#include "widget/RectBox.hpp"
#include "Context.hpp"

#include <Viewport.hpp>

namespace triglav::ui_core {

RectBox::RectBox(Context& context, State state, IWidget* parent) :
    ContainerWidget(context, parent),
    m_state(std::move(state))
{
}

Vector2 RectBox::desired_size(const Vector2 available_size) const
{
   if (m_cached_available_size_size.has_value() && *m_cached_available_size_size == available_size)
      return m_cached_size;

   m_cached_available_size_size.emplace(available_size);
   m_cached_size = m_content->desired_size(available_size);
   return m_cached_size;
}

void RectBox::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   if (!do_regions_intersect(dimensions, cropping_mask)) {
      if (m_rect_name != 0) {
         m_context.viewport().remove_rectangle(m_rect_name);
         m_rect_name = 0;
      }
      return;
   }

   if (m_rect_name != 0) {
      m_context.viewport().set_rectangle_dims(m_rect_name, dimensions, cropping_mask);
   } else {
      m_rect_name = m_context.viewport().add_rectangle(Rectangle{
         .rect = dimensions,
         .color = m_state.color,
         .border_radius = m_state.border_radius,
         .border_color = m_state.border_color,
         .crop = cropping_mask,
         .border_width = m_state.border_width,
      });
   }

   m_content->add_to_viewport(dimensions, cropping_mask);
}

void RectBox::remove_from_viewport()
{
   m_content->remove_from_viewport();
   m_context.viewport().remove_rectangle_safe(m_rect_name);
}

void RectBox::set_color(Vector4 color)
{
   if (m_state.color == color)
      return;
   if (m_rect_name == 0)
      return;

   m_state.color = color;
   m_context.viewport().set_rectangle_color(m_rect_name, color);
}

void RectBox::on_child_state_changed(IWidget& widget)
{
   m_cached_available_size_size.reset();
   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(widget);
   }
}

void RectBox::on_event(const Event& event)
{
   m_content->on_event(event);
}

}// namespace triglav::ui_core
