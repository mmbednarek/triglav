#include "Dialog.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include <spdlog/spdlog.h>

namespace triglav::desktop_ui {

using namespace name_literals;
using namespace string_literals;

Dialog::Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
               render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager, const Vector2u dimensions,
               const StringView title) :
    m_glyphCache(glyphCache),
    m_resourceManager(resourceManager),
    m_surface(display.create_surface(title, dimensions, desktop::WindowAttribute::Default)),
    m_graphicsSurface(GAPI_CHECK(instance.create_surface(*m_surface))),
    m_resourceStorage(device),
    m_renderSurface(device, *m_surface, m_graphicsSurface, m_resourceStorage, dimensions, graphics_api::PresentMode::Immediate),
    m_uiViewport(dimensions),
    m_updateUiJob(device, m_glyphCache, m_uiViewport, m_resourceManager),
    m_pipelineCache(device, m_resourceManager),
    m_jobGraph(device, m_resourceManager, m_pipelineCache, m_resourceStorage, dimensions),
    m_context(m_uiViewport, m_glyphCache, m_resourceManager),
    TG_CONNECT(*m_surface, OnClose, on_close),
    TG_CONNECT(*m_surface, OnMouseEnter, on_mouse_enter),
    TG_CONNECT(*m_surface, OnMouseMove, on_mouse_move),
    TG_CONNECT(*m_surface, OnMouseWheelTurn, on_mouse_wheel_turn),
    TG_CONNECT(*m_surface, OnMouseButtonIsPressed, on_mouse_button_is_pressed),
    TG_CONNECT(*m_surface, OnMouseButtonIsReleased, on_mouse_button_is_released),
    TG_CONNECT(*m_surface, OnKeyIsPressed, on_key_is_pressed),
    TG_CONNECT(*m_surface, OnTextInput, on_text_input),
    TG_CONNECT(*m_surface, OnResize, on_resize)
{
}

Dialog::Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::ISurface& parentSurface,
               render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager, const Vector2u dimensions,
               const Vector2i offset) :
    m_glyphCache(glyphCache),
    m_resourceManager(resourceManager),
    m_surface(parentSurface.create_popup(dimensions, offset, desktop::WindowAttribute::None)),
    m_graphicsSurface(GAPI_CHECK(instance.create_surface(*m_surface))),
    m_resourceStorage(device),
    m_renderSurface(device, *m_surface, m_graphicsSurface, m_resourceStorage, dimensions, graphics_api::PresentMode::Immediate),
    m_uiViewport(dimensions),
    m_updateUiJob(device, m_glyphCache, m_uiViewport, m_resourceManager),
    m_pipelineCache(device, m_resourceManager),
    m_jobGraph(device, m_resourceManager, m_pipelineCache, m_resourceStorage, dimensions),
    m_context(m_uiViewport, m_glyphCache, m_resourceManager),
    TG_CONNECT(*m_surface, OnClose, on_close),
    TG_CONNECT(*m_surface, OnMouseEnter, on_mouse_enter),
    TG_CONNECT(*m_surface, OnMouseMove, on_mouse_move),
    TG_CONNECT(*m_surface, OnMouseWheelTurn, on_mouse_wheel_turn),
    TG_CONNECT(*m_surface, OnMouseButtonIsPressed, on_mouse_button_is_pressed),
    TG_CONNECT(*m_surface, OnMouseButtonIsReleased, on_mouse_button_is_released),
    TG_CONNECT(*m_surface, OnKeyIsPressed, on_key_is_pressed),
    TG_CONNECT(*m_surface, OnTextInput, on_text_input),
    TG_CONNECT(*m_surface, OnResize, on_resize)
{
}

void Dialog::initialize()
{
   if (m_isInitialized)
      return;
   if (m_rootWidget == nullptr)
      return;

   m_rootWidget->add_to_viewport({0, 0, m_surface->dimension().x, m_surface->dimension().y},
                                 {0, 0, m_surface->dimension().x, m_surface->dimension().y});

   auto& updateUiCtx = m_jobGraph.add_job("update_ui"_name);
   m_updateUiJob.build_job(updateUiCtx);

   auto& renderCtx = m_jobGraph.add_job("render_dialog"_name);
   this->build_rendering_job(renderCtx);

   m_jobGraph.add_dependency_to_previous_frame("update_ui"_name, "update_ui"_name);

   renderer::RenderSurface::add_present_jobs(m_jobGraph, "render_dialog"_name);

   m_jobGraph.add_dependency("render_dialog"_name, "update_ui"_name);

   m_jobGraph.build_jobs("render_dialog"_name);

   m_renderSurface.recreate_present_jobs();

   m_isInitialized = true;
}

void Dialog::uninitialize() const
{
   if (m_rootWidget != nullptr) {
      m_rootWidget->remove_from_viewport();
   }
}

void Dialog::build_rendering_job(render_core::BuildContext& ctx)
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));

   ctx.begin_render_pass("splash_screen"_name, "core.color_out"_name);

   m_updateUiJob.render_ui(ctx);

   ctx.end_render_pass();

   ctx.export_texture("core.color_out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void Dialog::update()
{
   if (!m_isInitialized)
      return;
   if (!m_uiViewport.should_redraw())
      return;

   m_renderSurface.await_for_frame(m_frameIndex);

   m_jobGraph.build_semaphores();

   m_updateUiJob.prepare_frame(m_jobGraph, m_frameIndex);

   m_jobGraph.execute("render_dialog"_name, m_frameIndex, nullptr);

   m_renderSurface.present(m_jobGraph, m_frameIndex);

   m_frameIndex = (m_frameIndex + 1) % 3;
}

void Dialog::on_close()
{
   m_shouldClose = true;
}

void Dialog::on_mouse_enter(const Vector2 position)
{
   m_mousePosition = position;
}

void Dialog::on_mouse_move(const Vector2 position)
{
   m_mousePosition = position;

   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseMoved;
   event.mousePosition = position;
   event.globalMousePosition = position;
   event.parentSize = m_renderSurface.resolution();
   m_rootWidget->on_event(event);
}

void Dialog::on_mouse_wheel_turn(const float amount) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseScrolled;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_renderSurface.resolution();
   event.data = ui_core::Event::Scroll{amount};
   m_rootWidget->on_event(event);
}

void Dialog::on_mouse_button_is_pressed(desktop::MouseButton button) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MousePressed;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_renderSurface.resolution();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_rootWidget->on_event(event);
}

void Dialog::on_mouse_button_is_released(desktop::MouseButton button) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::MouseReleased;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_renderSurface.resolution();
   event.data.emplace<ui_core::Event::Mouse>(button);
   m_rootWidget->on_event(event);
}

void Dialog::on_key_is_pressed(desktop::Key key) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::KeyPressed;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_renderSurface.resolution();
   event.data.emplace<ui_core::Event::Keyboard>(key);
   m_rootWidget->on_event(event);
}

void Dialog::on_text_input(const Rune rune) const
{
   if (m_rootWidget == nullptr)
      return;

   ui_core::Event event;
   event.eventType = ui_core::Event::Type::TextInput;
   event.mousePosition = m_mousePosition;
   event.globalMousePosition = m_mousePosition;
   event.parentSize = m_renderSurface.resolution();
   event.data.emplace<ui_core::Event::TextInput>(rune);
   m_rootWidget->on_event(event);
}

void Dialog::on_resize(const Vector2i size)
{
   spdlog::info("Resize event: {} {}", size.x, size.y);
};

ui_core::IWidget& Dialog::set_root_widget(ui_core::IWidgetPtr&& content)
{
   if (m_rootWidget != nullptr) {
      m_rootWidget->remove_from_viewport();
   }

   m_rootWidget = std::move(content);

   return *m_rootWidget;
}

desktop::ISurface& Dialog::surface() const
{
   return *m_surface;
}

bool Dialog::should_close() const
{
   return m_shouldClose;
}

}// namespace triglav::desktop_ui
