#pragma once

#include "triglav/String.hpp"

#include <array>

#define TG_LAUNCH_STAGES          \
   TG_STAGE(GraphicsDeviceInit)   \
   TG_STAGE(LoadInitialResources) \
   TG_STAGE(LoadAllResources)

namespace triglav::launcher {

enum class LaunchStage
{
   Uninitialized,
#define TG_STAGE(name) name,
   TG_LAUNCH_STAGES
#undef TG_STAGE
      Complete,
};

[[nodiscard]] StringView launch_stage_to_string(LaunchStage stage);

#define TG_STAGE(name) class name##Stage;
TG_LAUNCH_STAGES
#undef TG_STAGE

class Application;

class IStage
{
 public:
   explicit IStage(Application& application);

   virtual ~IStage() = default;

   virtual void tick() = 0;

 protected:
   Application& m_application;
};


}// namespace triglav::launcher
