#pragma once

#include "Launcher.hpp"

#include "triglav/Event.hpp"
#include "triglav/engine/Engine.hpp"
#include "triglav/resource/ResourceManager.hpp"

namespace triglav::launcher {

class LoadInitialResourcesStage : public IStage
{
 public:
   using Self = LoadInitialResourcesStage;

   explicit LoadInitialResourcesStage(Application& app);

   void tick() override;
   void on_engine_ready();

 private:
   std::atomic_bool m_completed{};
   TG_OPT_SINK(engine::Engine, OnEngineReady);
};

}// namespace triglav::launcher
