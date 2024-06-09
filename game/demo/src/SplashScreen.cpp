#include "SplashScreen.h"

#include <utility>

namespace demo {

using namespace triglav::name_literals;

using triglav::graphics_api::AttachmentAttribute;
using triglav::graphics_api::ColorFormat;
using triglav::graphics_api::ColorSpace;
using triglav::graphics_api::RenderTargetBuilder;
using triglav::graphics_api::Resolution;
using triglav::graphics_api::ClearValue;
using triglav::graphics_api::Color;
using triglav::graphics_api::WorkType;
using triglav::graphics_api::Framebuffer;
using triglav::graphics_api::Swapchain;
using triglav::graphics_api::RenderTarget;
using triglav::u32;

constexpr Resolution g_splashScreenResolution{1024, 360};
constexpr ColorFormat g_splashScreenColorFormat{GAPI_FORMAT(BGRA, sRGB)};

namespace {

std::vector<Framebuffer> create_framebuffers(const Swapchain& swapchain, const RenderTarget& renderTarget)
{
   std::vector<Framebuffer> result{};
   const auto frameCount = swapchain.frame_count();
   for (u32 i = 0; i < frameCount; ++i) {
      result.emplace_back(GAPI_CHECK(renderTarget.create_swapchain_framebuffer(swapchain, i)));
   }
   return result;
}

}

SplashScreen::SplashScreen(triglav::graphics_api::Surface& surface, triglav::graphics_api::Device& device, triglav::resource::ResourceManager& resourceManager) :
    m_surface(surface),
    m_device(device),
    m_resourceManager(resourceManager),
    m_swapchain(GAPI_CHECK(m_device.create_swapchain(m_surface, GAPI_FORMAT(BGRA, sRGB), ColorSpace::sRGB, g_splashScreenResolution))),
    m_renderTarget(GAPI_CHECK(RenderTargetBuilder(m_device)
                                 .attachment("output"_name,
                                             AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                AttachmentAttribute::StoreImage | AttachmentAttribute::Presentable,
                                             g_splashScreenColorFormat)
                                 .build())),
    m_framebuffers(create_framebuffers(m_swapchain, m_renderTarget)),
    m_commandList(GAPI_CHECK(m_device.queue_manager().create_command_list(WorkType::Graphics))),
    m_frameReadySemaphore(GAPI_CHECK(m_device.create_semaphore())),
    m_targetSemaphore(GAPI_CHECK(m_device.create_semaphore())),
    m_frameFinishedFence(GAPI_CHECK(m_device.create_fence()))
{
}

void SplashScreen::update()
{
   m_frameFinishedFence.await();

   const auto framebufferIndex = m_swapchain.get_available_framebuffer(m_frameReadySemaphore);

   auto& framebuffer = m_framebuffers[framebufferIndex];

   GAPI_CHECK_STATUS(m_commandList.begin());

   std::array<ClearValue, 1> clearValues{
      {{Color{0, 0, 0, 0}}},
   };
   m_commandList.begin_render_pass(framebuffer, clearValues);
   m_commandList.end_render_pass();

   GAPI_CHECK_STATUS(m_commandList.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list(m_commandList, m_frameReadySemaphore, m_targetSemaphore, m_frameFinishedFence));

   GAPI_CHECK_STATUS(m_swapchain.present(m_targetSemaphore, framebufferIndex));
}

void SplashScreen::await()
{
   m_frameFinishedFence.await();
}

}// namespace demo
