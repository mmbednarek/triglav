#include "RootWindow.hpp"

#include "RenderViewport.hpp"
#include "triglav/render_core/BuildContext.hpp"

#include <spdlog/spdlog.h>

namespace triglav::editor {

using namespace name_literals;
using namespace string_literals;

constexpr Vector2 DEFAULT_DIMENSIONS{1280, 720};

RootWindow::RootWindow(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
                       render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_surface(display.create_surface("Triglav Editor"_strv, DEFAULT_DIMENSIONS, desktop::WindowAttribute::Default)),
    m_graphicsSurface(GAPI_CHECK(instance.create_surface(*m_surface))),
    m_resourceStorage(device),
    m_renderSurface(device, *m_surface, m_graphicsSurface, m_resourceStorage, DEFAULT_DIMENSIONS, graphics_api::PresentMode::Immediate),
    m_widgetRenderer(*m_surface, glyphCache, resourceManager, device),
    m_pipelineCache(device, resourceManager),
    m_jobGraph(device, resourceManager, m_pipelineCache, m_resourceStorage, DEFAULT_DIMENSIONS),
    TG_CONNECT(*m_surface, OnClose, on_close),
    TG_CONNECT(*m_surface, OnResize, on_resize)
{
}

void RootWindow::initialize()
{
   if (m_isInitialized)
      return;
   if (m_widgetRenderer.is_empty())
      return;

   m_widgetRenderer.add_widget_to_viewport(m_surface->dimension());

   auto& updateUiCtx = m_jobGraph.add_job("update_ui"_name);
   m_widgetRenderer.create_update_job(updateUiCtx);

   auto& renderCtx = m_jobGraph.add_job("render_dialog"_name);
   this->build_rendering_job(renderCtx);

   m_jobGraph.add_dependency_to_previous_frame("update_ui"_name, "update_ui"_name);

   renderer::RenderSurface::add_present_jobs(m_jobGraph, "render_dialog"_name);

   m_jobGraph.add_dependency("render_dialog"_name, "update_ui"_name);

   m_jobGraph.build_jobs("render_dialog"_name);

   m_renderSurface.recreate_present_jobs();

   m_isInitialized = true;
}

void RootWindow::uninitialize() const
{
   m_widgetRenderer.remove_widget_from_viewport();
}

void RootWindow::build_rendering_job(render_core::BuildContext& ctx)
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));
   m_widgetRenderer.create_render_job(ctx, "core.color_out"_name);

   if (m_renderViewport != nullptr) {
      m_renderViewport->build_render_job(ctx);
   }

   ctx.export_texture("core.color_out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void RootWindow::update()
{
   if (!m_isInitialized)
      return;
   if (!m_widgetRenderer.ui_viewport().should_redraw())
      return;

   if (m_shouldUpdateViewport) {
      m_device.await_all();

      auto& renderCtx = m_jobGraph.replace_job("render_dialog"_name);
      this->build_rendering_job(renderCtx);
      m_jobGraph.rebuild_job("render_dialog"_name);
      m_shouldUpdateViewport = false;

      m_renderSurface.recreate_present_jobs();
   }

   m_renderSurface.await_for_frame(m_frameIndex);

   m_jobGraph.build_semaphores();

   m_widgetRenderer.prepare_resources(m_jobGraph, m_frameIndex);

   m_jobGraph.execute("render_dialog"_name, m_frameIndex, nullptr);

   m_renderSurface.present(m_jobGraph, m_frameIndex);

   m_frameIndex = (m_frameIndex + 1) % 3;
}

void RootWindow::on_close()
{
   m_shouldClose = true;
}

void RootWindow::on_resize(const Vector2i size)
{
   spdlog::info("Resize event: {} {}", size.x, size.y);

   m_device.await_all();

   m_jobGraph.set_screen_size(size);
   m_widgetRenderer.ui_viewport().set_dimensions(size);

   auto& updateUiCtx = m_jobGraph.replace_job("update_ui"_name);
   m_widgetRenderer.create_update_job(updateUiCtx);
   m_jobGraph.rebuild_job("update_ui"_name);

   auto& renderCtx = m_jobGraph.replace_job("render_dialog"_name);
   this->build_rendering_job(renderCtx);
   m_jobGraph.rebuild_job("render_dialog"_name);

   m_renderSurface.recreate_swapchain(size);
   m_widgetRenderer.add_widget_to_viewport(size);
}

void RootWindow::set_render_viewport(RenderViewport* viewport)
{
   m_renderViewport = viewport;
   if (m_isInitialized) {
      m_shouldUpdateViewport = true;
   }
}

desktop::ISurface& RootWindow::surface() const
{
   return *m_surface;
}

bool RootWindow::should_close() const
{
   return m_shouldClose;
}

}// namespace triglav::editor
