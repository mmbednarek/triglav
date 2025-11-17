#include "LoadAllResourcesStage.hpp"

#include "Application.hpp"

#include "triglav/resource/PathManager.hpp"

namespace triglav::launcher {

LoadAllResourcesStage::LoadAllResourcesStage(Application& app) :
    IStage(app)
{
   TG_CONNECT_OPT(*app.m_resource_manager, OnLoadedAssets, on_loaded_assets);
   app.m_resource_manager->load_asset_list(resource::PathManager::the().content_path().sub("index.yaml"));
}

void LoadAllResourcesStage::tick()
{
   if (m_completed.load()) {
      m_application.complete_stage();
   }
}

void LoadAllResourcesStage::on_loaded_assets()
{
   m_completed.store(true);
}

}// namespace triglav::launcher
