#include "LoadInitialResourcesStage.hpp"

#include "Application.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/resource/PathManager.hpp"

namespace triglav::launcher {

LoadInitialResourcesStage::LoadInitialResourcesStage(Application& app) :
    IStage(app)
{
   app.m_resource_manager = std::make_unique<resource::ResourceManager>(*app.m_gfx_device, app.m_font_manager);
   app.m_glyph_cache = std::make_unique<render_core::GlyphCache>(*app.m_gfx_device, *app.m_resource_manager);

   TG_CONNECT_OPT(*app.m_resource_manager, OnLoadedAssets, on_loaded_assets);
   const auto proj_path = resource::PathManager::the().project_content_path();
   assert(proj_path.has_value());
   app.m_resource_manager->load_asset_list(proj_path->sub("index_base.yaml"));
}

void LoadInitialResourcesStage::tick()
{
   if (m_completed.load()) {
      m_application.complete_stage();
   }
}

void LoadInitialResourcesStage::on_loaded_assets()
{
   m_completed.store(true);
}

}// namespace triglav::launcher