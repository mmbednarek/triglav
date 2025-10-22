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

   void on_loaded_assets();

 private:
   std::atomic_bool m_completed{};
   TG_OPT_SINK(resource::ResourceManager, OnLoadedAssets);
};

}// namespace triglav::launcher
