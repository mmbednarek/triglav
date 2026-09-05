#include "LoadAllResourcesStage.hpp"

#include "Application.hpp"

#include "triglav/engine/Engine.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/project/ProjectManager.hpp"

namespace triglav::launcher {
using namespace name_literals;

LoadAllResourcesStage::LoadAllResourcesStage(Application& app) :
    IStage(app)
{
   TG_CONNECT_OPT(engine::Engine::the().resource_manager(), OnLoadedAssets, on_loaded_assets);
   const auto index_path = project::this_project() == "triglav_editor"_name ? "editor/index.yaml"_rc : "index.yaml"_rc;
   const auto proj_path = project::PathManager::the().translate_path(index_path);
   m_load_index = engine::Engine::the().resource_manager().load_asset_list(proj_path);
   assert(m_load_index != resource::ERROR_LOADING_ASSET);
}

void LoadAllResourcesStage::tick()
{
   if (m_completed.load()) {
      m_application.complete_stage();
   }
}

void LoadAllResourcesStage::on_loaded_assets(const resource::LoadIndex load_index)
{
   if (m_load_index != load_index)
      return;
   m_load_index = resource::ERROR_LOADING_ASSET;
   m_completed.store(true);
}

}// namespace triglav::launcher
