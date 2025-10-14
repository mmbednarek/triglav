#pragma once

#include "Launcher.hpp"

#include "triglav/event/Delegate.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::launcher {

class LoadInitialResourcesStage : public IStage
{
 public:
   using Self = LoadInitialResourcesStage;

   explicit LoadInitialResourcesStage(Application& app);

   void tick() override;
   void on_loaded_assets();

 private:
   std::atomic_bool m_completed{};

   TG_OPT_SINK(resource::ResourceManager, OnLoadedAssets);
};

}// namespace triglav::launcher
