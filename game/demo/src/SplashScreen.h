#pragma once

#include "triglav/desktop/ISurface.hpp"
#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Swapchain.h"
#include "triglav/graphics_api/RenderTarget.h"
#include <memory>

namespace demo {

class SplashScreen {
 public:
   SplashScreen(triglav::graphics_api::Surface& surface, triglav::graphics_api::Device& device);


 private:
   triglav::graphics_api::Surface& m_surface;
   triglav::graphics_api::Device& m_device;
   triglav::graphics_api::Swapchain m_swapchain;
   triglav::graphics_api::RenderTarget m_renderTarget;
};

}