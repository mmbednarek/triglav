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
   const auto [minResolution, maxResolution] = device.get_surface_resolution_limits(surface);
   resolution.x = std::clamp(resolution.x, minResolution.width, maxResolution.width);
   resolution.y = std::clamp(resolution.y, minResolution.height, maxResolution.height);
   return resolution;
}

}// namespace

RenderSurface::RenderSurface(graphics_api::Device& device, desktop::ISurface& desktopSurface, graphics_api::Surface& surface,
                             render_core::ResourceStorage& resourceStorage, const Vector2u resolution,
                             const graphics_api::PresentMode presentMode) :
    m_device(device),
    m_desktopSurface(desktopSurface),
    m_surface(surface),
    m_resourceStorage(resourceStorage),
    m_resolution(create_viewport_resolution(device, surface, resolution)),
    m_presentMode(presentMode),
    m_swapchain(GAPI_CHECK(device.create_swapchain(surface, GAPI_FORMAT(BGRA, sRGB), graphics_api::ColorSpace::sRGB,
                                                   {resolution.x, resolution.y}, m_presentMode))),
    m_frameFences{GAPI_CHECK(device.create_fence()), GAPI_CHECK(device.create_fence()), GAPI_CHECK(device.create_fence())}
{
}

void RenderSurface::add_present_jobs(render_core::JobGraph& jobGraph, const Name externalJob)
{
   jobGraph.add_external_job("job.acquire_swapchain_image"_name);
   jobGraph.add_external_job("job.copy_present_image"_name);
   jobGraph.add_external_job("job.present_swapchain_image"_name);

   jobGraph.add_dependency("job.copy_present_image"_name, "job.acquire_swapchain_image"_name);
   jobGraph.add_dependency("job.copy_present_image"_name, externalJob);
   jobGraph.add_dependency("job.present_swapchain_image"_name, "job.copy_present_image"_name);
}

void RenderSurface::await_for_frame(const u32 frameIndex) const
{
   m_frameFences[frameIndex].await();
}

void RenderSurface::present(render_core::JobGraph& jobGraph, const u32 frameIndex)
{
   if (m_mustRecreateSwapchain) {
      recreate_swapchain(m_desktopSurface.dimension());
   }

   const auto [framebufferIndex, mustRecreate] = GAPI_CHECK(m_swapchain.get_available_framebuffer(
      jobGraph.semaphore("job.copy_present_image"_name, "job.acquire_swapchain_image"_name, frameIndex)));
   if (mustRecreate) {
      m_mustRecreateSwapchain = true;
   }

   GAPI_CHECK_STATUS(m_device.submit_command_list(m_prePresentCommands[framebufferIndex * render_core::FRAMES_IN_FLIGHT_COUNT + frameIndex],
                                                  jobGraph.wait_semaphores("job.copy_present_image"_name, frameIndex),
                                                  jobGraph.signal_semaphores("job.copy_present_image"_name, frameIndex),
                                                  &m_frameFences[frameIndex], graphics_api::WorkType::Presentation));

   const auto status = m_swapchain.present(jobGraph.wait_semaphores("job.present_swapchain_image"_name, frameIndex), framebufferIndex);
   if (status == graphics_api::Status::OutOfDateSwapchain) {
      m_mustRecreateSwapchain = true;
   } else {
      GAPI_CHECK_STATUS(status);
   }
}

void RenderSurface::recreate_present_jobs()
{
   m_prePresentCommands.clear();
   m_prePresentCommands.reserve(m_swapchain.textures().size() * render_core::FRAMES_IN_FLIGHT_COUNT);

   for (const auto& swapchainTexture : m_swapchain.textures()) {
      for (const u32 frameIndex : Range(0, render_core::FRAMES_IN_FLIGHT_COUNT)) {
         auto cmdList = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Graphics | graphics_api::WorkType::Transfer));
         GAPI_CHECK_STATUS(cmdList.begin());

         graphics_api::TextureBarrierInfo inBarrier{};
         inBarrier.texture = &swapchainTexture;
         inBarrier.sourceState = graphics_api::TextureState::Undefined;
         inBarrier.targetState = graphics_api::TextureState::TransferDst;
         inBarrier.baseMipLevel = 0;
         inBarrier.mipLevelCount = 1;
         cmdList.texture_barrier(graphics_api::PipelineStage::Entrypoint, graphics_api::PipelineStage::Transfer, inBarrier);

         cmdList.copy_texture(m_resourceStorage.texture("core.color_out"_name, frameIndex), graphics_api::TextureState::TransferSrc,
                              swapchainTexture, graphics_api::TextureState::TransferDst);

         graphics_api::TextureBarrierInfo outBarrier{};
         outBarrier.texture = &swapchainTexture;
         outBarrier.sourceState = graphics_api::TextureState::TransferDst;
         outBarrier.targetState = graphics_api::TextureState::Present;
         outBarrier.baseMipLevel = 0;
         outBarrier.mipLevelCount = 1;
         cmdList.texture_barrier(graphics_api::PipelineStage::Transfer, graphics_api::PipelineStage::End, outBarrier);

         GAPI_CHECK_STATUS(cmdList.finish());

         m_prePresentCommands.emplace_back(std::move(cmdList));
      }
   }
}

void RenderSurface::recreate_swapchain(const Vector2u newResolution)
{
   m_device.await_all();
   m_swapchain = GAPI_CHECK(m_device.create_swapchain(m_surface, m_swapchain.color_format(), graphics_api::ColorSpace::sRGB,
                                                      {newResolution.x, newResolution.y}, m_presentMode, &m_swapchain));

   m_resolution = newResolution;

   RenderSurface::recreate_present_jobs();
   m_mustRecreateSwapchain = false;
}

Vector2u RenderSurface::resolution() const
{
   return m_resolution;
}

}// namespace triglav::renderer
