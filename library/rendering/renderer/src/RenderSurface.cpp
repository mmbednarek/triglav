#include "RenderSurface.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/render_core/JobGraph.hpp"

#include <triglav/desktop/ISurface.hpp>

namespace triglav::renderer {

using namespace name_literals;

namespace {

Vector2u create_viewport_resolution(const graphics_api::Device& device, const graphics_api::Surface& surface, Vector2u resolution)
{
   const auto [min_resolution, max_resolution] = device.get_surface_resolution_limits(surface);
   resolution.x = std::clamp(resolution.x, min_resolution.width, max_resolution.width);
   resolution.y = std::clamp(resolution.y, min_resolution.height, max_resolution.height);
   return resolution;
}

}// namespace

RenderSurface::RenderSurface(graphics_api::Device& device, desktop::ISurface& desktop_surface, graphics_api::Surface& surface,
                             render_core::ResourceStorage& resource_storage, const Vector2u resolution,
                             const graphics_api::PresentMode present_mode) :
    m_device(device),
    m_desktop_surface(desktop_surface),
    m_surface(surface),
    m_resource_storage(resource_storage),
    m_resolution(create_viewport_resolution(device, surface, resolution)),
    m_present_mode(present_mode),
    m_swapchain(GAPI_CHECK(device.create_swapchain(surface, GAPI_FORMAT(BGRA, sRGB), graphics_api::ColorSpace::sRGB,
                                                   {resolution.x, resolution.y}, m_present_mode))),
    m_frame_fences{GAPI_CHECK(device.create_fence()), GAPI_CHECK(device.create_fence()), GAPI_CHECK(device.create_fence())}
{
}

void RenderSurface::add_present_jobs(render_core::JobGraph& job_graph, const Name external_job)
{
   job_graph.add_external_job("job.acquire_swapchain_image"_name);
   job_graph.add_external_job("job.copy_present_image"_name);
   job_graph.add_external_job("job.present_swapchain_image"_name);

   job_graph.add_dependency("job.copy_present_image"_name, "job.acquire_swapchain_image"_name);
   job_graph.add_dependency("job.copy_present_image"_name, external_job);
   job_graph.add_dependency("job.present_swapchain_image"_name, "job.copy_present_image"_name);
}

void RenderSurface::await_for_frame(const u32 frame_index) const
{
   m_frame_fences[frame_index].await();
}

void RenderSurface::present(render_core::JobGraph& job_graph, const u32 frame_index)
{
   if (m_must_recreate_swapchain) {
      recreate_swapchain(m_desktop_surface.dimension());
   }

   const auto [framebuffer_index, must_recreate] = GAPI_CHECK(m_swapchain.get_available_framebuffer(
      job_graph.semaphore("job.copy_present_image"_name, "job.acquire_swapchain_image"_name, frame_index)));
   if (must_recreate) {
      m_must_recreate_swapchain = true;
   }

   GAPI_CHECK_STATUS(
      m_device.submit_command_list(m_pre_present_commands[framebuffer_index * render_core::FRAMES_IN_FLIGHT_COUNT + frame_index],
                                   job_graph.wait_semaphores("job.copy_present_image"_name, frame_index),
                                   job_graph.signal_semaphores("job.copy_present_image"_name, frame_index), &m_frame_fences[frame_index],
                                   graphics_api::WorkType::Presentation));

   const auto status = m_swapchain.present(job_graph.wait_semaphores("job.present_swapchain_image"_name, frame_index), framebuffer_index);
   if (status == graphics_api::Status::OutOfDateSwapchain) {
      m_must_recreate_swapchain = true;
   } else {
      GAPI_CHECK_STATUS(status);
   }
}

void RenderSurface::recreate_present_jobs()
{
   m_pre_present_commands.clear();
   m_pre_present_commands.reserve(m_swapchain.textures().size() * render_core::FRAMES_IN_FLIGHT_COUNT);

   for (const auto& swapchain_texture : m_swapchain.textures()) {
      for (const u32 frame_index : Range(0, render_core::FRAMES_IN_FLIGHT_COUNT)) {
         auto cmd_list = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Graphics));
         GAPI_CHECK_STATUS(cmd_list.begin());

         graphics_api::TextureBarrierInfo in_barrier{};
         in_barrier.texture = &swapchain_texture;
         in_barrier.source_state = graphics_api::TextureState::Undefined;
         in_barrier.target_state = graphics_api::TextureState::TransferDst;
         in_barrier.base_mip_level = 0;
         in_barrier.mip_level_count = 1;
         cmd_list.texture_barrier(graphics_api::PipelineStage::Entrypoint, graphics_api::PipelineStage::Transfer, in_barrier);

         cmd_list.copy_texture(m_resource_storage.texture("core.color_out"_name, frame_index), graphics_api::TextureState::TransferSrc,
                               swapchain_texture, graphics_api::TextureState::TransferDst);

         graphics_api::TextureBarrierInfo out_barrier{};
         out_barrier.texture = &swapchain_texture;
         out_barrier.source_state = graphics_api::TextureState::TransferDst;
         out_barrier.target_state = graphics_api::TextureState::Present;
         out_barrier.base_mip_level = 0;
         out_barrier.mip_level_count = 1;
         cmd_list.texture_barrier(graphics_api::PipelineStage::Transfer, graphics_api::PipelineStage::End, out_barrier);

         GAPI_CHECK_STATUS(cmd_list.finish());

         m_pre_present_commands.emplace_back(std::move(cmd_list));
      }
   }
}

void RenderSurface::recreate_swapchain(const Vector2u new_resolution)
{
   m_device.await_all();
   m_swapchain = GAPI_CHECK(m_device.create_swapchain(m_surface, m_swapchain.color_format(), graphics_api::ColorSpace::sRGB,
                                                      {new_resolution.x, new_resolution.y}, m_present_mode, &m_swapchain));

   m_resolution = new_resolution;

   RenderSurface::recreate_present_jobs();
   m_must_recreate_swapchain = false;
}

Vector2u RenderSurface::resolution() const
{
   return m_resolution;
}

}// namespace triglav::renderer
