#include "LoadInitialResourcesStage.hpp"

#include "Application.hpp"

#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/resource/PathManager.hpp"

namespace triglav::launcher {

LoadInitialResourcesStage::LoadInitialResourcesStage(Application& app) :
    IStage(app)
{
   app.m_resourceManager = std::make_unique<resource::ResourceManager>(*app.m_gfxDevice, app.m_fontManager);
   app.m_glyphCache = std::make_unique<render_core::GlyphCache>(*app.m_gfxDevice, *app.m_resourceManager);

   TG_CONNECT_OPT(*app.m_resourceManager, OnLoadedAssets, on_loaded_assets);
   app.m_resourceManager->load_asset_list(resource::PathManager::the().content_path().sub("index_base.yaml"));
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