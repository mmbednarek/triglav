#include "LoadAllResourcesStage.hpp"

#include "Application.hpp"

namespace triglav::launcher {

LoadAllResourcesStage::LoadAllResourcesStage(Application& app) :
    IStage(app)
{
}

void LoadAllResourcesStage::tick()
{
   m_application.complete_stage();
}

}// namespace triglav::launcher
