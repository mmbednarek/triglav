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

void Context::set_active_widget(ui_core::IWidget* active_widget, const Vector4 active_area, std::set<Event::Type> redirected_events)
{
   if (m_active_widget != nullptr) {
      Event deactivate_event{};
      deactivate_event.event_type = Event::Type::Deactivated;
      deactivate_event.global_mouse_position = {};
      deactivate_event.mouse_position = {};
      deactivate_event.parent_size = {};
      m_active_widget->on_event(deactivate_event);
   }

   if (const auto it = std::ranges::find(m_activating_widgets, active_widget); it != m_activating_widgets.end()) {
      m_active_widget_id = it - m_activating_widgets.begin();
      log_debug("activating widget: {}", m_active_widget_id);
   }

   m_active_widget = active_widget;
   m_active_area = active_area;
   m_redirected_events = std::move(redirected_events);
}

ui_core::IWidget* Context::active_widget() const
{
   return m_active_widget;
}

Vector4 Context::active_area() const
{
   return m_active_area;
}

bool Context::should_redirect_event(const Event::Type event_type) const
{
   return m_redirected_events.contains(event_type);
}

void Context::add_activating_widget(IWidget* widget)
{
   if (const auto it = std::ranges::find(m_activating_widgets, widget); it == m_activating_widgets.end()) {
      m_active_widget_id = m_activating_widgets.size();
      m_activating_widgets.emplace_back(widget);
   }
}

void Context::remove_activating_widget(IWidget* widget)
{
   if (const auto it = std::ranges::find(m_activating_widgets, widget); it != m_activating_widgets.end()) {
      m_activating_widgets.erase(it);
   }
}

void Context::toggle_active_widget() const
{
   const auto id = (m_active_widget_id + 1) % m_activating_widgets.size();
   Event activate_event{};
   activate_event.event_type = Event::Type::Activated;
   activate_event.global_mouse_position = {};
   activate_event.mouse_position = {};
   activate_event.parent_size = {};
   m_activating_widgets[id]->on_event(activate_event);
}

}// namespace triglav::ui_core
