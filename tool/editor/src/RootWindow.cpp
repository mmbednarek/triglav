#include "RootWindow.hpp"

#include "DefaultRenderOverlay.hpp"
#include "level_editor/RenderViewport.hpp"
#include "triglav/render_core/BuildContext.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace string_literals;
using namespace render_core::literals;

constexpr Vector2 DEFAULT_DIMENSIONS{1280, 720};

RootWindow::RootWindow(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
                       render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager) :
    m_device(device),
    m_resource_manager(resource_manager),
    m_surface(display.create_surface("Triglav Editor"_strv, DEFAULT_DIMENSIONS, desktop::WindowAttribute::Default)),
    m_graphics_surface(GAPI_CHECK(instance.create_surface(*m_surface))),
    m_resource_storage(device),
    m_render_surface(device, *m_surface, m_graphics_surface, m_resource_storage, DEFAULT_DIMENSIONS, graphics_api::PresentMode::Fifo),
    m_widget_renderer(*m_surface, glyph_cache, resource_manager, device, *this),
    m_pipeline_cache(device, resource_manager),
    m_job_graph(device, resource_manager, m_pipeline_cache, m_resource_storage, DEFAULT_DIMENSIONS),
    TG_CONNECT(*m_surface, OnClose, on_close),
    TG_CONNECT(*m_surface, OnResize, on_resize)
{
}

void RootWindow::initialize()
{
   if (m_is_initialized)
      return;
   if (m_widget_renderer.is_empty())
      return;

   m_widget_renderer.add_widget_to_viewport(m_surface->dimension());

   auto& update_viewport_ctx = m_job_graph.add_job(renderer::UpdateViewParamsJob::JobName);
   this->render_overlay().build_update_job(update_viewport_ctx);
   m_job_graph.add_dependency_to_previous_frame(renderer::UpdateViewParamsJob::JobName);

   auto& render_viewport_ctx = m_job_graph.add_job("render_viewport"_name, rect_size(this->render_overlay().dimensions()));
   this->render_overlay().build_render_job(render_viewport_ctx);
   m_job_graph.add_dependency("render_viewport"_name, renderer::UpdateViewParamsJob::JobName);

   auto& update_ui_ctx = m_job_graph.add_job("update_ui"_name);
   m_widget_renderer.create_update_job(update_ui_ctx);
   m_job_graph.add_dependency_to_previous_frame("update_ui"_name, "update_ui"_name);

   auto& render_dialog_ctx = m_job_graph.add_job("render_dialog"_name);
   this->build_rendering_job(render_dialog_ctx);
   m_job_graph.add_dependency("render_dialog"_name, "render_viewport"_name);
   m_job_graph.add_dependency("render_dialog"_name, "update_ui"_name);

   renderer::RenderSurface::add_present_jobs(m_job_graph, "render_dialog"_name);

   m_job_graph.build_jobs("render_dialog"_name);

   m_render_surface.recreate_present_jobs();

   m_is_initialized = true;
}

void RootWindow::uninitialize() const
{
   m_widget_renderer.remove_widget_from_viewport();
}

void RootWindow::build_rendering_job(render_core::BuildContext& ctx)
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));
   m_widget_renderer.create_render_job(ctx, "core.color_out"_name);

   ctx.copy_texture_region("render_viewport.out"_external, {0, 0}, "core.color_out"_name,
                           rect_position(this->render_overlay().dimensions()), rect_size(this->render_overlay().dimensions()));

   ctx.export_texture("core.color_out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void RootWindow::update()
{
   if (!m_is_initialized)
      return;
   // if (!m_widget_renderer.ui_viewport().should_redraw())
   //    return;

   if (m_should_update_viewport) {
      m_device.await_all();

      auto& render_viewport_ctx = m_job_graph.replace_job("render_viewport"_name, rect_size(this->render_overlay().dimensions()));
      this->render_overlay().build_render_job(render_viewport_ctx);
      m_job_graph.rebuild_job("render_viewport"_name);

      auto& render_dialog_ctx = m_job_graph.replace_job("render_dialog"_name);
      this->build_rendering_job(render_dialog_ctx);
      m_job_graph.rebuild_job("render_dialog"_name);

      m_should_update_viewport = false;

      m_render_surface.recreate_present_jobs();
   }

   m_render_surface.await_for_frame(m_frame_index);

   m_job_graph.build_semaphores();

   this->render_overlay().update(m_job_graph, m_frame_index, 0.017f);

   m_widget_renderer.prepare_resources(m_job_graph, m_frame_index);

   m_job_graph.execute("render_dialog"_name, m_frame_index, nullptr);

   m_render_surface.present(m_job_graph, m_frame_index);

   m_frame_index = (m_frame_index + 1) % 3;
}

void RootWindow::on_close()
{
   m_should_close = true;
}

void RootWindow::on_resize(const Vector2i size)
{
   m_device.await_all();

   m_job_graph.set_screen_size(size);
   m_widget_renderer.ui_viewport().set_dimensions(size);

   m_widget_renderer.add_widget_to_viewport(size);
   m_should_update_viewport = false;// already updating here

   auto& update_ui_ctx = m_job_graph.replace_job("update_ui"_name);
   m_widget_renderer.create_update_job(update_ui_ctx);
   m_job_graph.rebuild_job("update_ui"_name);

   auto& render_viewport_ctx = m_job_graph.replace_job("render_viewport"_name, rect_size(this->render_overlay().dimensions()));
   this->render_overlay().build_render_job(render_viewport_ctx);
   m_job_graph.rebuild_job("render_viewport"_name);

   auto& render_ctx = m_job_graph.replace_job("render_dialog"_name);
   this->build_rendering_job(render_ctx);
   m_job_graph.rebuild_job("render_dialog"_name);

   m_render_surface.recreate_swapchain(size);
}

void RootWindow::set_render_overlay(IRenderOverlay* overlay)
{
   m_render_overlay = overlay;
   if (m_is_initialized) {
      m_should_update_viewport = true;
   }
}

IRenderOverlay& RootWindow::render_overlay() const
{
   static DefaultRenderOverlay default_overlay;
   if (m_render_overlay == nullptr) {
      return default_overlay;
   }
   return *m_render_overlay;
}

graphics_api::Device& RootWindow::device() const
{
   return m_device;
}

desktop::ISurface& RootWindow::surface() const
{
   return *m_surface;
}

resource::ResourceManager& RootWindow::resource_manager() const
{
   return m_resource_manager;
}

bool RootWindow::should_close() const
{
   return m_should_close;
}

void RootWindow::recreate_render_jobs()
{
   m_should_update_viewport = true;
}

}// namespace triglav::editor
