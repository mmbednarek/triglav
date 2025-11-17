#include "GraphicsDeviceInitStage.hpp"

#include "Application.hpp"

namespace triglav::launcher {

using namespace string_literals;

GraphicsDeviceInitStage::GraphicsDeviceInitStage(Application& app) :
    IStage(app)
{
   app.m_gfx_instance =
      std::make_unique<graphics_api::Instance>(GAPI_CHECK(triglav::graphics_api::Instance::create_instance(&app.m_display)));
   const auto surface = app.m_display.create_surface("Temporary Window"_strv, {32, 32}, desktop::WindowAttribute::Default);
   const auto surface_gfx = GAPI_CHECK(app.m_gfx_instance->create_surface(*surface));

   app.m_gfx_device = GAPI_CHECK(app.m_gfx_instance->create_device(&surface_gfx, triglav::graphics_api::DevicePickStrategy::PreferDedicated,
                                                                   triglav::graphics_api::DeviceFeature::None));
}

void GraphicsDeviceInitStage::tick()
{
   m_application.complete_stage();
}

}// namespace triglav::launcher