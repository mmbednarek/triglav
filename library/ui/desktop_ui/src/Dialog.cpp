#include "Dialog.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include "WidgetRenderer.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;
using namespace string_literals;

Dialog::Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::IDisplay& display,
               render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager, const Vector2u dimensions,
               const StringView title) :
    Dialog(instance, device, glyph_cache, resource_manager, dimensions,
           display.create_surface(title, dimensions, desktop::WindowAttribute::Default))
{
}

Dialog::Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, desktop::ISurface& parent_surface,
               render_core::GlyphCache& glyph_cache, resource::ResourceManager& resource_manager, const Vector2u dimensions,
               const Vector2i offset) :
    Dialog(instance, device, glyph_cache, resource_manager, dimensions,
           parent_surface.create_popup(dimensions, offset, desktop::WindowAttribute::None))
{
}

Dialog::Dialog(const graphics_api::Instance& instance, graphics_api::Device& device, render_core::GlyphCache& glyph_cache,
               resource::ResourceManager& resource_manager, const Vector2u dimensions, std::shared_ptr<desktop::ISurface> surface) :
    m_device(device),
    m_surface(std::move(surface)),
    m_graphics_surface(GAPI_CHECK(instance.create_surface(*m_surface))),
    m_resource_storage(device),
    m_render_surface(device, *m_surface, m_graphics_surface, m_resource_storage, dimensions, graphics_api::PresentMode::Immediate),
    m_widget_renderer(*m_surface, glyph_cache, resource_manager, device, *this),
    m_pipeline_cache(device, resource_manager),
    m_job_graph(device, resource_manager, m_pipeline_cache, m_resource_storage, dimensions),
    TG_CONNECT(*m_surface, OnClose, on_close),
    TG_CONNECT(*m_surface, OnResize, on_resize)
{
}

void Dialog::initialize()
{
   if (m_is_initialized)
      return;
   if (m_widget_renderer.is_empty())
      return;

   m_widget_renderer.add_widget_to_viewport(m_surface->dimension());

   auto& update_ui_ctx = m_job_graph.add_job("update_ui"_name);
   m_widget_renderer.create_update_job(update_ui_ctx);

   auto& render_ctx = m_job_graph.add_job("render_dialog"_name);
   this->build_rendering_job(render_ctx);

   m_job_graph.add_dependency_to_previous_frame("update_ui"_name, "update_ui"_name);

   renderer::RenderSurface::add_present_jobs(m_job_graph, "render_dialog"_name);

   m_job_graph.add_dependency("render_dialog"_name, "update_ui"_name);

   m_job_graph.build_jobs("render_dialog"_name);

   m_render_surface.recreate_present_jobs();

   m_is_initialized = true;
}

void Dialog::uninitialize() const
{
   m_widget_renderer.remove_widget_from_viewport();
}

void Dialog::build_rendering_job(render_core::BuildContext& ctx)
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));
   m_widget_renderer.create_render_job(ctx, "core.color_out"_name);
   ctx.export_texture("core.color_out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void Dialog::update()
{
   if (!m_is_initialized)
      return;
   if (!m_widget_renderer.ui_viewport().should_redraw())
      return;
   if (m_should_rebuild) {
      auto& render_ctx = m_job_graph.replace_job("render_dialog"_name);
      this->build_rendering_job(render_ctx);
      m_render_surface.recreate_present_jobs();
      m_should_rebuild = false;
   }

   m_render_surface.await_for_frame(m_frame_index);

   m_job_graph.build_semaphores();

   m_widget_renderer.prepare_resources(m_job_graph, m_frame_index);

   m_job_graph.execute("render_dialog"_name, m_frame_index, nullptr);

   m_render_surface.present(m_job_graph, m_frame_index);

   m_frame_index = (m_frame_index + 1) % 3;
}

void Dialog::on_close()
{
   m_should_close = true;
}

void Dialog::on_resize(const Vector2i size)
{
   m_device.await_all();

   m_job_graph.set_screen_size(size);
   m_widget_renderer.ui_viewport().set_dimensions(size);

   auto& update_ui_ctx = m_job_graph.replace_job("update_ui"_name);
   m_widget_renderer.create_update_job(update_ui_ctx);
   m_job_graph.rebuild_job("update_ui"_name);

   auto& render_ctx = m_job_graph.replace_job("render_dialog"_name);
   this->build_rendering_job(render_ctx);
   m_job_graph.rebuild_job("render_dialog"_name);

   m_render_surface.recreate_swapchain(size);
   m_widget_renderer.add_widget_to_viewport(size);
};

WidgetRenderer& Dialog::widget_renderer()
{
   return m_widget_renderer;
}

void Dialog::recreate_render_jobs()
{
   m_should_rebuild = true;
}

desktop::ISurface& Dialog::surface() const
{
   return *m_surface;
}

bool Dialog::should_close() const
{
   return m_should_close;
}

}// namespace triglav::desktop_ui
