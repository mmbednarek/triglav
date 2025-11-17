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
   RenderSurface(graphics_api::Device& device, desktop::ISurface& desktop_surface, graphics_api::Surface& surface,
                 render_core::ResourceStorage& resource_storage, Vector2u resolution, graphics_api::PresentMode present_mode);

   static void add_present_jobs(render_core::JobGraph& job_graph, Name external_job);

   void await_for_frame(u32 frame_index) const;
   void present(render_core::JobGraph& job_graph, u32 frame_index);
   void recreate_swapchain(Vector2u new_resolution);
   void recreate_present_jobs();

   [[nodiscard]] Vector2u resolution() const;

 private:
   graphics_api::Device& m_device;
   desktop::ISurface& m_desktop_surface;
   graphics_api::Surface& m_surface;
   render_core::ResourceStorage& m_resource_storage;
   Vector2u m_resolution{};
   graphics_api::PresentMode m_present_mode{graphics_api::PresentMode::Fifo};
   graphics_api::Swapchain m_swapchain;
   std::vector<graphics_api::CommandList> m_pre_present_commands;
   std::array<graphics_api::Fence, render_core::FRAMES_IN_FLIGHT_COUNT> m_frame_fences;
   bool m_must_recreate_swapchain{false};
};

}// namespace triglav::renderer