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

constexpr triglav::Vector2u g_splashScreenResolution{1024, 360};

SplashScreen::SplashScreen(triglav::desktop::ISurface& surface, triglav::graphics_api::Surface& graphicsSurface,
                           triglav::graphics_api::Device& device, triglav::resource::ResourceManager& resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_resourceStorage(m_device),
    m_renderSurface(m_device, surface, graphicsSurface, m_resourceStorage, g_splashScreenResolution, PresentMode::Fifo),
    m_glyphCache(device, m_resourceManager),
    m_pipelineCache(m_device, m_resourceManager),
    m_uiViewport({g_splashScreenResolution.x, g_splashScreenResolution.y}),
    m_updateUiJob(m_device, m_glyphCache, m_uiViewport, m_resourceManager),
    m_jobGraph(m_device, m_resourceManager, m_pipelineCache, m_resourceStorage, {g_splashScreenResolution.x, g_splashScreenResolution.y})
{
   triglav::ui_core::Rectangle statusBg{
      .rect = {40.0f, 225.0f, g_splashScreenResolution.x - 40.0f, 275.0f},
      .color = {0.0f, 0.12f, 0.33f, 1.0f},
   };
   m_statusBgId = m_uiViewport.add_rectangle(std::move(statusBg));

   triglav::ui_core::Rectangle statusFg{
      .rect = {40.0f, 225.0f, 50.0f, 275.0f},
      .color = {0.05f, 0.29f, 0.67f, 1.0f},
   };
   m_statusFgId = m_uiViewport.add_rectangle(std::move(statusFg));

   triglav::ui_core::Text titleText{
      .content = "Triglav Render Demo",
      .typefaceName = "cantarell/bold.typeface"_rc,
      .fontSize = 36,
      .position = {64.0f, 80.0f},
      .color = {0.13f, 0.39f, 0.78f, 1.0f},
      .crop = {0, 0, g_splashScreenResolution.x, g_splashScreenResolution.y},
   };
   m_titleId = m_uiViewport.add_text(std::move(titleText));

   triglav::ui_core::Text descText{
      .content = "Loading Resources",
      .typefaceName = "cantarell.typeface"_rc,
      .fontSize = 24,
      .position = {64.0f, 160.0f},
      .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
      .crop = {0, 0, g_splashScreenResolution.x, g_splashScreenResolution.y},
   };
   m_descId = m_uiViewport.add_text(std::move(descText));

   auto& updateUiCtx = m_jobGraph.add_job("update_ui"_name);
   m_updateUiJob.build_job(updateUiCtx);

   auto& renderCtx = m_jobGraph.add_job("render_status"_name);
   this->build_rendering_job(renderCtx);

   m_jobGraph.add_dependency_to_previous_frame("update_ui"_name, "update_ui"_name);

   triglav::renderer::RenderSurface::add_present_jobs(m_jobGraph, "render_status"_name);

   m_jobGraph.add_dependency("render_status"_name, "update_ui"_name);

   m_jobGraph.build_jobs("render_status"_name);

   m_renderSurface.recreate_present_jobs();

   TG_CONNECT_OPT(m_resourceManager, OnStartedLoadingAsset, on_started_loading_asset);
   TG_CONNECT_OPT(m_resourceManager, OnFinishedLoadingAsset, on_finished_loading_asset);
}

void SplashScreen::update()
{
   m_renderSurface.await_for_frame(m_frameIndex);

   m_jobGraph.build_semaphores();

   m_updateUiJob.prepare_frame(m_jobGraph, m_frameIndex);

   m_jobGraph.execute("render_status"_name, m_frameIndex, nullptr);

   m_renderSurface.present(m_jobGraph, m_frameIndex);

   m_frameIndex = (m_frameIndex + 1) % 3;
}

void SplashScreen::on_close()
{
   sink_OnStartedLoadingAsset->disconnect();

   m_renderSurface.await_for_frame(m_frameIndex);

   m_device.await_all();
}

void SplashScreen::build_rendering_job(triglav::render_core::BuildContext& ctx)
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));

   ctx.begin_render_pass("splash_screen"_name, "core.color_out"_name);

   m_updateUiJob.render_ui(ctx);

   ctx.end_render_pass();

   ctx.export_texture("core.color_out"_name, triglav::graphics_api::PipelineStage::Transfer,
                      triglav::graphics_api::TextureState::TransferSrc, triglav::graphics_api::TextureUsage::TransferSrc);
}

void SplashScreen::on_started_loading_asset(const triglav::ResourceName resourceName)
{
   std::unique_lock lk{m_statusMutex};
   m_pendingResources.insert(resourceName);
   this->update_loaded_resource(resourceName);
}

void SplashScreen::on_finished_loading_asset(const triglav::ResourceName resourceName, const u32 loadedAssets, const u32 totalAssets)
{
   std::unique_lock lk{m_statusMutex};
   this->update_process_bar(static_cast<float>(loadedAssets) / static_cast<float>(totalAssets));

   m_pendingResources.erase(resourceName);
   if (m_displayedResource == resourceName && not m_pendingResources.empty()) {
      const auto index = rand() % m_pendingResources.size();
      auto it = m_pendingResources.begin();
      std::advance(it, index);
      this->update_loaded_resource(*it);
   }
}

void SplashScreen::update_process_bar(const float progress)
{
   const triglav::Vector4 dims{40.0f, 225.0f, triglav::lerp(50.0f, g_splashScreenResolution.x - 40.0f, progress), 275.0f};
   m_uiViewport.set_rectangle_dims(m_statusFgId, dims, {0, 0, g_splashScreenResolution.x, g_splashScreenResolution.y});
}

void SplashScreen::update_loaded_resource(const triglav::ResourceName name)
{
   m_displayedResource = name;
   const auto msg = triglav::format("Loading resource {}", m_resourceManager.lookup_name(name).value_or("unknown"));
   m_uiViewport.set_text_content(m_descId, msg.view());
}

}// namespace demo
