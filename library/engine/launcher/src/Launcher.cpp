#include "Launcher.hpp"

namespace triglav::launcher {

using namespace string_literals;

constexpr std::array g_launchStageNames{
   "Uninitialized"_strv,
#define TG_STAGE(name) #name##_strv,
   TG_LAUNCH_STAGES
#undef TG_STAGE
   "Complete"_strv,
};

StringView launch_stage_to_string(const LaunchStage stage)
{
   return g_launchStageNames[static_cast<MemorySize>(stage)];
}

IStage::IStage(Application& application) :
    m_application(application)
{
}

}// namespace triglav::launcher
