#include "WidgetRenderer.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

WidgetRenderer::WidgetRenderer(desktop::ISurface& surface, render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager,
                               graphics_api::Device& device) :
    m_surface(surface),
    m_uiViewport(surface.dimension()),
    m_updateUiJob(device, glyphCache, m_uiViewport, resourceManager),
    m_context(m_uiViewport, glyphCache, resourceManager),
    TG_CONNECT(m_surface, OnMouseEnter, on_mouse_enter),
    TG_CONNECT(m_surface, OnMouseMove, on_mouse_move),
    TG_CONNECT(m_surface, OnMouseWheelTurn, on_mouse_wheel_turn),
    TG_CONNECT(m_surface, OnMouseButtonIsPressed, on_mouse_button_is_pressed),
    TG_CONNECT(m_surface, OnMouseButtonIsReleased, on_mouse_button_is_released),
    TG_CONNECT(m_surface, OnKeyIsPressed, on_key_is_pressed),
    TG_CONNECT(m_surface, OnKeyIsReleased, on_key_is_released),
    TG_CONNECT(m_surface, OnTextInput, on_text_input)
{
}

void WidgetRenderer::create_update_job(render_core::BuildContext& ctx) const
{
   m_updateUiJob.build_job(ctx);
}

void WidgetRenderer::create_render_job(render_core::BuildContext& ctx, Name output_render_target)
{
   ctx.begin_render_pass("dialog_render_ui"_name, output_render_target);

   m_updateUiJob.render_ui(ctx);

   ctx.end_render_pass();
}

void WidgetRenderer::prepare_resources(render_core::JobGraph& graph, const u32 frame_index)
{
   m_updateUiJob.prepare_frame(graph, frame_index);
}

void WidgetRenderer::add_widget_to_viewport(const Vector2i resolution) const
{
   m_rootWidget->add_to_viewport({0, 0, resolution.x, resolution.y}, {0, 0, resolution.x, resolution.y});
}

void WidgetRenderer::remove_widget_from_viewport() const
{
   if (m_rootWidget != nullptr) {
      m_rootWidget->remove_from_viewport();
   }
}

void WidgetRenderer::on_mouse_enter(const Vector2 position)
{
   m_mousePosition = position;
}

void WidgetRenderer::on_mouse_move(const Vector2 position)
{
   m_mousePosition = position;

   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseMoved;
   event.mousePosition = position;
   event.globalMousePosition = position;
   event.parentSize = m_surface.dimension();
   m_rootWidget->on_event(event);
}

void WidgetRenderer::on_mouse_wheel_turn(const float amount) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseScrolled;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_surface.dimension();
   event.data = ui_core::Event::Scroll{amount};
   m_rootWidget->on_event(event);
}

void WidgetRenderer::on_mouse_button_is_pressed(desktop::MouseButton button) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MousePressed;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_surface.dimension();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_rootWidget->on_event(event);
}

void WidgetRenderer::on_mouse_button_is_released(desktop::MouseButton button) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseReleased;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_surface.dimension();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_rootWidget->on_event(event);
}

void WidgetRenderer::on_key_is_pressed(desktop::Key key) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::KeyPressed;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_surface.dimension();
   event.data.emplace<ui_core::Event::Keyboard>(key);
   m_rootWidget->on_event(event);
}

void WidgetRenderer::on_key_is_released(desktop::Key key) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::KeyReleased;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_surface.dimension();
   event.data.emplace<ui_core::Event::Keyboard>(key);
   m_rootWidget->on_event(event);
}

void WidgetRenderer::on_text_input(const Rune rune) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::TextInput;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_surface.dimension();
   event.data.emplace<ui_core::Event::TextInput>(rune);
   m_rootWidget->on_event(event);
}

ui_core::IWidget& WidgetRenderer::set_root_widget(ui_core::IWidgetPtr&& content)
{
   if (m_rootWidget != nullptr) {
      m_rootWidget->remove_from_viewport();
   }

   m_rootWidget = std::move(content);

   return *m_rootWidget;
}

bool WidgetRenderer::is_empty() const
{
   return m_rootWidget == nullptr;
}

ui_core::Viewport& WidgetRenderer::ui_viewport()
{
   return m_uiViewport;
}

}// namespace triglav::desktop_ui
