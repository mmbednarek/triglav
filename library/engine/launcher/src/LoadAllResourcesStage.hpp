#pragma once

#include "Launcher.hpp"

#include "triglav/resource/ResourceManager.hpp"

#include <atomic>

namespace triglav::launcher {

class LoadAllResourcesStage : public IStage
{
 public:
   using Self = LoadAllResourcesStage;

   explicit LoadAllResourcesStage(Application& app);

   void tick() override;

   void on_loaded_assets(resource::LoadIndex load_index);

 private:
   std::atomic_bool m_completed{};
   resource::LoadIndex m_load_index = resource::ERROR_LOADING_ASSET;
   TG_SINK(OnLoadedAssets);
};

}// namespace triglav::launcher
