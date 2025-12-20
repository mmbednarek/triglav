#include "SplashScreen.hpp"

#include "triglav/Format.hpp"
#include "triglav/String.hpp"
#include "triglav/renderer/Renderer.hpp"


namespace demo {

using namespace triglav::name_literals;

using triglav::u32;
using triglav::graphics_api::AttachmentAttribute;
using triglav::graphics_api::ClearValue;
using triglav::graphics_api::Color;
using triglav::graphics_api::ColorFormat;
using triglav::graphics_api::ColorSpace;
using triglav::graphics_api::PresentMode;
using triglav::graphics_api::Resolution;
using triglav::graphics_api::Status;
using triglav::graphics_api::Swapchain;
using triglav::graphics_api::WorkType;

constexpr triglav::Vector2u g_splash_screen_resolution{1024, 360};

SplashScreen::SplashScreen(triglav::desktop::ISurface& surface, triglav::graphics_api::Surface& graphics_surface,
                           triglav::graphics_api::Device& device, triglav::resource::ResourceManager& resource_manager) :
    m_device(device),
    m_resource_manager(resource_manager),
    m_resource_storage(m_device),
    m_render_surface(m_device, surface, graphics_surface, m_resource_storage, g_splash_screen_resolution, PresentMode::Fifo),
    m_glyph_cache(device, m_resource_manager),
    m_pipeline_cache(m_device, m_resource_manager),
    m_ui_viewport({g_splash_screen_resolution.x, g_splash_screen_resolution.y}),
    m_update_ui_job(m_device, m_glyph_cache, m_ui_viewport, m_resource_manager, *this),
    m_job_graph(m_device, m_resource_manager, m_pipeline_cache, m_resource_storage,
                {g_splash_screen_resolution.x, g_splash_screen_resolution.y})
{
   triglav::ui_core::Rectangle status_bg{
      .rect = {40.0f, 225.0f, g_splash_screen_resolution.x - 40.0f, 275.0f},
      .color = {0.0f, 0.12f, 0.33f, 1.0f},
   };
   m_status_bg_id = m_ui_viewport.add_rectangle(std::move(status_bg));

   triglav::ui_core::Rectangle status_fg{
      .rect = {40.0f, 225.0f, 50.0f, 275.0f},
      .color = {0.05f, 0.29f, 0.67f, 1.0f},
   };
   m_status_fg_id = m_ui_viewport.add_rectangle(std::move(status_fg));

   triglav::ui_core::Text title_text{
      .content = "Triglav Render Demo",
      .typeface_name = "fonts/cantarell/bold.typeface"_rc,
      .font_size = 36,
      .position = {64.0f, 80.0f},
      .color = {0.13f, 0.39f, 0.78f, 1.0f},
      .crop = {0, 0, g_splash_screen_resolution.x, g_splash_screen_resolution.y},
   };
   m_title_id = m_ui_viewport.add_text(std::move(title_text));

   triglav::ui_core::Text desc_text{
      .content = "Loading Resources",
      .typeface_name = "fonts/cantarell/regular.typeface"_rc,
      .font_size = 24,
      .position = {64.0f, 160.0f},
      .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
      .crop = {0, 0, g_splash_screen_resolution.x, g_splash_screen_resolution.y},
   };
   m_desc_id = m_ui_viewport.add_text(std::move(desc_text));

   auto& update_ui_ctx = m_job_graph.add_job("update_ui"_name);
   m_update_ui_job.build_job(update_ui_ctx);

   auto& render_ctx = m_job_graph.add_job("render_status"_name);
   this->build_rendering_job(render_ctx);

   m_job_graph.add_dependency_to_previous_frame("update_ui"_name, "update_ui"_name);

   triglav::renderer::RenderSurface::add_present_jobs(m_job_graph, "render_status"_name);

   m_job_graph.add_dependency("render_status"_name, "update_ui"_name);

   m_job_graph.build_jobs("render_status"_name);

   m_render_surface.recreate_present_jobs();

   TG_CONNECT_OPT(m_resource_manager, OnStartedLoadingAsset, on_started_loading_asset);
   TG_CONNECT_OPT(m_resource_manager, OnFinishedLoadingAsset, on_finished_loading_asset);
}

void SplashScreen::update()
{
   m_render_surface.await_for_frame(m_frame_index);

   m_job_graph.build_semaphores();

   m_update_ui_job.prepare_frame(m_job_graph, m_frame_index);

   m_job_graph.execute("render_status"_name, m_frame_index, nullptr);

   m_render_surface.present(m_job_graph, m_frame_index);

   m_frame_index = (m_frame_index + 1) % 3;
}

void SplashScreen::on_close()
{
   sink_OnStartedLoadingAsset->disconnect();

   m_render_surface.await_for_frame(m_frame_index);

   m_device.await_all();
}

void SplashScreen::build_rendering_job(triglav::render_core::BuildContext& ctx)
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));

   ctx.begin_render_pass("splash_screen"_name, "core.color_out"_name);

   m_update_ui_job.render_ui(ctx);

   ctx.end_render_pass();

   ctx.export_texture("core.color_out"_name, triglav::graphics_api::PipelineStage::Transfer,
                      triglav::graphics_api::TextureState::TransferSrc, triglav::graphics_api::TextureUsage::TransferSrc);
}

void SplashScreen::on_started_loading_asset(const triglav::ResourceName resource_name)
{
   std::unique_lock lk{m_status_mutex};
   m_pending_resources.insert(resource_name);
   this->update_loaded_resource(resource_name);
}

void SplashScreen::on_finished_loading_asset(const triglav::ResourceName resource_name, const u32 loaded_assets, const u32 total_assets)
{
   std::unique_lock lk{m_status_mutex};
   this->update_process_bar(static_cast<float>(loaded_assets) / static_cast<float>(total_assets));

   m_pending_resources.erase(resource_name);
   if (m_displayed_resource == resource_name && not m_pending_resources.empty()) {
      const auto index = rand() % m_pending_resources.size();
      auto it = m_pending_resources.begin();
      std::advance(it, index);
      this->update_loaded_resource(*it);
   }
}

void SplashScreen::recreate_render_jobs()
{
   // unsupported
}

void SplashScreen::update_process_bar(const float progress)
{
   const triglav::Vector4 dims{40.0f, 225.0f, triglav::lerp(50.0f, g_splash_screen_resolution.x - 40.0f, progress), 275.0f};
   m_ui_viewport.set_rectangle_dims(m_status_fg_id, dims, {0, 0, g_splash_screen_resolution.x, g_splash_screen_resolution.y});
}

void SplashScreen::update_loaded_resource(const triglav::ResourceName name)
{
   m_displayed_resource = name;
   const auto msg = triglav::format("Loading resource {}", m_resource_manager.lookup_name(name).value_or("unknown"));
   m_ui_viewport.set_text_content(m_desc_id, msg.view());
}

}// namespace demo
