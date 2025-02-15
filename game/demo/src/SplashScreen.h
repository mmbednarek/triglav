#pragma once

#include "triglav/desktop/ISurface.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Swapchain.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/renderer/UpdateUserInterfaceJob.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <array>
#include <memory>
#include <set>
#include <mutex>

namespace demo {

class SplashScreen
{
 public:
   using Self = SplashScreen;

   SplashScreen(triglav::desktop::ISurface& surface, triglav::graphics_api::Surface& graphicsSurface, triglav::graphics_api::Device& device,
                triglav::resource::ResourceManager& resourceManager);

   void update();
   void on_close();
   void recreate_swapchain();

   void build_rendering_job(triglav::render_core::BuildContext& ctx);

   void on_started_loading_asset(triglav::ResourceName resourceName);
   void on_finished_loading_asset(triglav::ResourceName resourceName, triglav::u32 loadedAssets, triglav::u32 totalAssets);

 private:
   void update_process_bar(float progress);
   void update_loaded_resource(triglav::ResourceName name);

   triglav::desktop::ISurface& m_surface;
   triglav::graphics_api::Surface& m_graphicsSurface;
   triglav::graphics_api::Device& m_device;
   triglav::resource::ResourceManager& m_resourceManager;
   triglav::graphics_api::Resolution m_resolution;
   triglav::renderer::GlyphCache m_glyphCache;
   triglav::graphics_api::Swapchain m_swapchain;
   std::array<triglav::graphics_api::Fence, 3> m_frameFences;
   std::set<triglav::ResourceName> m_pendingResources;
   triglav::ResourceName m_displayedResource;
   triglav::render_core::PipelineCache m_pipelineCache;
   triglav::render_core::ResourceStorage m_resourceStorage;
   triglav::ui_core::Viewport m_uiViewport;
   triglav::renderer::UpdateUserInterfaceJob m_updateUiJob;
   triglav::render_core::JobGraph m_jobGraph;
   std::vector<triglav::graphics_api::CommandList> m_prePresentCommands;
   triglav::u32 m_frameIndex{0};
   std::mutex m_statusMutex;

   TG_OPT_SINK(triglav::resource::ResourceManager, OnStartedLoadingAsset);
   TG_OPT_SINK(triglav::resource::ResourceManager, OnFinishedLoadingAsset);
};

}// namespace demo