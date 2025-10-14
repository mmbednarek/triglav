#pragma once

#include "Launcher.hpp"

namespace triglav::launcher {

class LoadAllResourcesStage : public IStage
{
 public:
   explicit LoadAllResourcesStage(Application& app);

   void tick() override;
};

}// namespace triglav::launcher
