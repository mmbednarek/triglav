#pragma once

#include "triglav/desktop/ISurface.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Swapchain.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/render_core/JobGraph.hpp"
#include "triglav/renderer/RenderSurface.hpp"
#include "triglav/renderer/UpdateUserInterfaceJob.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <array>
#include <memory>
#include <mutex>
#include <set>

namespace demo {

class SplashScreen
{
 public:
   using Self = SplashScreen;

   SplashScreen(triglav::desktop::ISurface& surface, triglav::graphics_api::Surface& graphics_surface,
                triglav::graphics_api::Device& device, triglav::resource::ResourceManager& resource_manager);

   void update();
   void on_close();

   void build_rendering_job(triglav::render_core::BuildContext& ctx);

   void on_started_loading_asset(triglav::ResourceName resource_name);
   void on_finished_loading_asset(triglav::ResourceName resource_name, triglav::u32 loaded_assets, triglav::u32 total_assets);

 private:
   void update_process_bar(float progress);
   void update_loaded_resource(triglav::ResourceName name);

   triglav::graphics_api::Device& m_device;
   triglav::resource::ResourceManager& m_resource_manager;
   triglav::render_core::ResourceStorage m_resource_storage;
   triglav::renderer::RenderSurface m_render_surface;
   triglav::render_core::GlyphCache m_glyph_cache;
   std::set<triglav::ResourceName> m_pending_resources;
   triglav::ResourceName m_displayed_resource;
   triglav::render_core::PipelineCache m_pipeline_cache;
   triglav::ui_core::Viewport m_ui_viewport;
   triglav::renderer::UpdateUserInterfaceJob m_update_ui_job;
   triglav::render_core::JobGraph m_job_graph;
   triglav::u32 m_frame_index{0};
   std::mutex m_status_mutex;
   triglav::ui_core::RectId m_status_bg_id{};
   triglav::ui_core::RectId m_status_fg_id{};
   triglav::ui_core::TextId m_title_id{};
   triglav::ui_core::TextId m_desc_id{};

   TG_OPT_SINK(triglav::resource::ResourceManager, OnStartedLoadingAsset);
   TG_OPT_SINK(triglav::resource::ResourceManager, OnFinishedLoadingAsset);
};

}// namespace demo