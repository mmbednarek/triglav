#include "Context.hpp"

#include "IWidget.hpp"

namespace triglav::ui_core {

Context::Context(Viewport& viewport, render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager) :
    m_viewport(viewport),
    m_glyphCache(glyphCache),
    m_resourceManager(resourceManager)
{
}

Viewport& Context::viewport() const
{
   return m_viewport;
}

render_core::GlyphCache& Context::glyph_cache() const
{
   return m_glyphCache;
}

resource::ResourceManager& Context::resource_manager() const
{
   return m_resourceManager;
}

void Context::set_active_widget(ui_core::IWidget* active_widget, const Vector4 active_area)
{
   if (m_activeWidget != nullptr) {
      Event deactivate_event{};
      deactivate_event.eventType = Event::Type::Deactivated;
      deactivate_event.globalMousePosition = {};
      deactivate_event.mousePosition = {};
      deactivate_event.parentSize = {};
      m_activeWidget->on_event(deactivate_event);
   }

   m_activeWidget = active_widget;
   m_activeArea = active_area;
}

ui_core::IWidget* Context::active_widget() const
{
   return m_activeWidget;
}

Vector4 Context::active_area() const
{
   return m_activeArea;
}

}// namespace triglav::ui_core
