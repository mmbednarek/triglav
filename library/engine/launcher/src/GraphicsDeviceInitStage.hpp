#pragma once

#include "Launcher.hpp"

namespace triglav::launcher {

class GraphicsDeviceInitStage : public IStage
{
 public:
   explicit GraphicsDeviceInitStage(Application& app);

   void tick() override;
};

}// namespace triglav::launcher