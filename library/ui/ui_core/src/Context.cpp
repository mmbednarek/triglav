#include "Context.hpp"

#include "IWidget.hpp"

namespace triglav::ui_core {

Context::Context(Viewport& viewport, render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager) :
    m_viewport(viewport),
    m_glyph_cache(glyph_cache),
    m_resource_manager(resource_manager)
{
}

Viewport& Context::viewport() const
{
   return m_viewport;
}

render_core::GlyphCache& Context::glyph_cache() const
{
   return m_glyph_cache;
}

resource::ResourceManager& Context::resource_manager() const
{
   return m_resource_manager;
}

void Context::set_active_widget(ui_core::IWidget* active_widget, const Vector4 active_area)
{
   if (m_active_widget != nullptr) {
      Event deactivate_event{};
      deactivate_event.event_type = Event::Type::Deactivated;
      deactivate_event.global_mouse_position = {};
      deactivate_event.mouse_position = {};
      deactivate_event.parent_size = {};
      m_active_widget->on_event(deactivate_event);
   }

   m_active_widget = active_widget;
   m_active_area = active_area;
}

ui_core::IWidget* Context::active_widget() const
{
   return m_active_widget;
}

Vector4 Context::active_area() const
{
   return m_active_area;
}

}// namespace triglav::ui_core
