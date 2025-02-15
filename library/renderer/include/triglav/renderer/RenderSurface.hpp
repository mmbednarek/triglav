#pragma once

#include "triglav/Math.hpp"
#include "triglav/graphics_api/Swapchain.hpp"
#include "triglav/graphics_api/Synchronization.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/render_core/ResourceStorage.hpp"

namespace triglav::graphics_api {
class Surface;
}

namespace triglav::render_core {
class JobGraph;
}

namespace triglav::renderer {

class RenderSurface
{
 public:
   RenderSurface(graphics_api::Device& device, desktop::ISurface& desktopSurface, graphics_api::Surface& surface,
                 render_core::ResourceStorage& resourceStorage, Vector2u resolution, graphics_api::PresentMode presentMode);

   static void add_present_jobs(render_core::JobGraph& jobGraph, Name externalJob);

   void await_for_frame(u32 frameIndex) const;
   void present(render_core::JobGraph& jobGraph, u32 frameIndex);
   void recreate_swapchain(Vector2u newResolution);
   void recreate_present_jobs();

   [[nodiscard]] Vector2u resolution() const;

 private:
   graphics_api::Device& m_device;
   desktop::ISurface& m_desktopSurface;
   graphics_api::Surface& m_surface;
   render_core::ResourceStorage& m_resourceStorage;
   Vector2u m_resolution{};
   graphics_api::PresentMode m_presentMode{graphics_api::PresentMode::Fifo};
   graphics_api::Swapchain m_swapchain;
   std::vector<graphics_api::CommandList> m_prePresentCommands;
   std::array<graphics_api::Fence, render_core::FRAMES_IN_FLIGHT_COUNT> m_frameFences;
   bool m_mustRecreateSwapchain{false};
};

}// namespace triglav::renderer