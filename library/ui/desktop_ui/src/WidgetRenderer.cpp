#include "WidgetRenderer.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

WidgetRenderer::WidgetRenderer(desktop::ISurface& surface, render_core::GlyphCache& glyph_cache,
                               resource::ResourceManager& resource_manager, graphics_api::Device& device,
                               render_core::IRenderer& renderer) :
    m_surface(surface),
    m_ui_viewport(surface.dimension()),
    m_update_ui_job(device, glyph_cache, m_ui_viewport, resource_manager, renderer),
    m_context(m_ui_viewport, glyph_cache, resource_manager),
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
   m_update_ui_job.build_job(ctx);
}

void WidgetRenderer::create_render_job(render_core::BuildContext& ctx, Name output_render_target)
{
   ctx.begin_render_pass("dialog_render_ui"_name, output_render_target);

   m_update_ui_job.render_ui(ctx);

   ctx.end_render_pass();
}

void WidgetRenderer::prepare_resources(render_core::JobGraph& graph, const u32 frame_index)
{
   m_update_ui_job.prepare_frame(graph, frame_index);
}

void WidgetRenderer::add_widget_to_viewport(const Vector2i resolution) const
{
   m_root_widget->add_to_viewport({0, 0, resolution.x, resolution.y}, {0, 0, resolution.x, resolution.y});
}

void WidgetRenderer::remove_widget_from_viewport() const
{
   if (m_root_widget != nullptr) {
      m_root_widget->remove_from_viewport();
   }
}

void WidgetRenderer::on_mouse_enter(const Vector2 position)
{
   m_mouse_position = position;
}

void WidgetRenderer::on_mouse_move(const Vector2 position)
{
   m_mouse_position = position;

   if (m_root_widget == nullptr)
      return;

   ui_core::Event event;
   event.event_type = ui_core::Event::Type::MouseMoved;
   event.mouse_position = position;
   event.global_mouse_position = position;
   event.widget_size = m_surface.dimension();
   m_root_widget->on_event(event);
}

void WidgetRenderer::on_mouse_wheel_turn(const float amount) const
{
   if (m_root_widget == nullptr)
      return;

   ui_core::Event event;
   event.event_type = ui_core::Event::Type::MouseScrolled;
   event.mouse_position = m_mouse_position;
   event.global_mouse_position = m_mouse_position;
   event.widget_size = m_surface.dimension();
   event.data = ui_core::Event::Scroll{amount};
   m_root_widget->on_event(event);
}

void WidgetRenderer::on_mouse_button_is_pressed(desktop::MouseButton button) const
{
   if (m_root_widget == nullptr)
      return;

   ui_core::Event event;
   event.event_type = ui_core::Event::Type::MousePressed;
   event.mouse_position = m_mouse_position;
   event.global_mouse_position = m_mouse_position;
   event.widget_size = m_surface.dimension();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_root_widget->on_event(event);
}

void WidgetRenderer::on_mouse_button_is_released(desktop::MouseButton button) const
{
   if (m_root_widget == nullptr)
      return;

   ui_core::Event event;
   event.event_type = ui_core::Event::Type::MouseReleased;
   event.mouse_position = m_mouse_position;
   event.global_mouse_position = m_mouse_position;
   event.widget_size = m_surface.dimension();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_root_widget->on_event(event);
}

void WidgetRenderer::on_key_is_pressed(desktop::Key key) const
{
   if (m_root_widget == nullptr)
      return;

   ui_core::Event event;
   event.event_type = ui_core::Event::Type::KeyPressed;
   event.mouse_position = m_mouse_position;
   event.global_mouse_position = m_mouse_position;
   event.widget_size = m_surface.dimension();
   event.data.emplace<ui_core::Event::Keyboard>(key);
   m_root_widget->on_event(event);
}

void WidgetRenderer::on_key_is_released(desktop::Key key) const
{
   if (m_root_widget == nullptr)
      return;

   ui_core::Event event;
   event.event_type = ui_core::Event::Type::KeyReleased;
   event.mouse_position = m_mouse_position;
   event.global_mouse_position = m_mouse_position;
   event.widget_size = m_surface.dimension();
   event.data.emplace<ui_core::Event::Keyboard>(key);
   m_root_widget->on_event(event);
}

void WidgetRenderer::on_text_input(const Rune rune) const
{
   if (m_root_widget == nullptr)
      return;

   ui_core::Event event;
   event.event_type = ui_core::Event::Type::TextInput;
   event.mouse_position = m_mouse_position;
   event.global_mouse_position = m_mouse_position;
   event.widget_size = m_surface.dimension();
   event.data.emplace<ui_core::Event::TextInput>(rune);
   m_root_widget->on_event(event);
}

ui_core::IWidget& WidgetRenderer::set_root_widget(ui_core::IWidgetPtr&& content)
{
   if (m_root_widget != nullptr) {
      m_root_widget->remove_from_viewport();
   }

   m_root_widget = std::move(content);

   return *m_root_widget;
}

bool WidgetRenderer::is_empty() const
{
   return m_root_widget == nullptr;
}

ui_core::Viewport& WidgetRenderer::ui_viewport()
{
   return m_ui_viewport;
}

ui_core::Context& WidgetRenderer::context()
{
   return m_context;
}

}// namespace triglav::desktop_ui
