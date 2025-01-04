#include "SplashScreen.h"

#include <fmt/core.h>

namespace demo {

using namespace triglav::name_literals;

using triglav::u32;
using triglav::graphics_api::AttachmentAttribute;
using triglav::graphics_api::ClearValue;
using triglav::graphics_api::Color;
using triglav::graphics_api::ColorFormat;
using triglav::graphics_api::ColorSpace;
using triglav::graphics_api::Framebuffer;
using triglav::graphics_api::PresentMode;
using triglav::graphics_api::RenderTarget;
using triglav::graphics_api::RenderTargetBuilder;
using triglav::graphics_api::Resolution;
using triglav::graphics_api::Status;
using triglav::graphics_api::Swapchain;
using triglav::graphics_api::WorkType;

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

}// namespace

SplashScreen::SplashScreen(triglav::desktop::ISurface& surface, triglav::graphics_api::Surface& graphicsSurface,
                           triglav::graphics_api::Device& device, triglav::resource::ResourceManager& resourceManager) :
    m_surface(surface),
    m_graphicsSurface(graphicsSurface),
    m_device(device),
    m_resourceManager(resourceManager),
    m_resolution(g_splashScreenResolution),
    m_glyphCache(device, m_resourceManager),
    m_swapchain(GAPI_CHECK(m_device.create_swapchain(m_graphicsSurface, GAPI_FORMAT(BGRA, sRGB), ColorSpace::sRGB, g_splashScreenResolution,
                                                     PresentMode::Fifo))),
    m_renderTarget(GAPI_CHECK(RenderTargetBuilder(m_device)
                                 .attachment("output"_name,
                                             AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                AttachmentAttribute::StoreImage | AttachmentAttribute::Presentable,
                                             g_splashScreenColorFormat)
                                 .build())),
    m_framebuffers(create_framebuffers(m_swapchain, m_renderTarget)),
    m_textRenderer(m_device, m_resourceManager, m_renderTarget, m_glyphCache),
    m_rectangleRenderer(m_device, m_renderTarget, m_resourceManager),
    m_commandList(GAPI_CHECK(m_device.queue_manager().create_command_list(WorkType::Graphics))),
    m_frameReadySemaphore(GAPI_CHECK(m_device.create_semaphore())),
    m_targetSemaphore(GAPI_CHECK(m_device.create_semaphore())),
    m_frameFinishedFence(GAPI_CHECK(m_device.create_fence())),
    m_textTitle(m_textRenderer.create_text_object("cantarell/bold.typeface"_rc, 36, "TRIGLAV RENDER DEMO")),
    m_textDesc(m_textRenderer.create_text_object("cantarell.typeface"_rc, 24, "Loading Resources")),
    m_statusBgRect(
       m_rectangleRenderer.create_rectangle({40.0f, 225.0f, g_splashScreenResolution.width - 40.0f, 275.0f}, {0.06f, 0.18f, 0.37f, 1.0f})),
    m_statusFgRect(m_rectangleRenderer.create_rectangle({40.0f, 225.0f, 50.0f, 275.0f}, {0.13f, 0.39f, 0.78f, 1.0f})),
    TG_CONNECT(m_resourceManager, OnStartedLoadingAsset, on_started_loading_asset),
    TG_CONNECT(m_resourceManager, OnFinishedLoadingAsset, on_finished_loading_asset)
{
}

void SplashScreen::update()
{
   m_frameFinishedFence.await();

   const auto [framebufferIndex, mustRecreate] = GAPI_CHECK(m_swapchain.get_available_framebuffer(m_frameReadySemaphore));
   if (mustRecreate) {
      this->recreate_swapchain();
   }

   auto& framebuffer = m_framebuffers[framebufferIndex];

   {
      std::unique_lock lk{m_updateTextMutex};
      if (not m_pendingDescChange.empty()) {
         m_textRenderer.update_text(m_textDesc, m_pendingDescChange);
         m_pendingDescChange.clear();
      }
      if (m_pendingStatusChange.has_value()) {
         m_rectangleRenderer.update_rectangle(m_statusFgRect,
                                              {40.0f, 225.0f, 40.0f + *m_pendingStatusChange * (m_resolution.width - 80.0f), 275.0f},
                                              {0.13f, 0.39f, 0.78f, 1.0f});
         m_pendingStatusChange.reset();
      }
   }

   GAPI_CHECK_STATUS(m_commandList.begin());

   std::array clearValues{
      ClearValue{Color{0, 0, 0, 0}},
   };
   m_commandList.begin_render_pass(framebuffer, clearValues);

   m_textRenderer.bind_pipeline(m_commandList);
   m_textRenderer.draw_text(m_commandList, m_textTitle, {m_resolution.width, m_resolution.height}, {64.0f, 80.0f},
                            {0.13f, 0.39f, 0.78f, 1.0f});

   m_textRenderer.draw_text(m_commandList, m_textDesc, {m_resolution.width, m_resolution.height}, {64.0f, 160.0f},
                            {1.0f, 1.0f, 1.0f, 1.0f});

   m_rectangleRenderer.begin_render(m_commandList);

   m_rectangleRenderer.draw(m_commandList, m_statusBgRect, {m_resolution.width, m_resolution.height});
   m_rectangleRenderer.draw(m_commandList, m_statusFgRect, {m_resolution.width, m_resolution.height});

   m_commandList.end_render_pass();

   GAPI_CHECK_STATUS(m_commandList.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list(m_commandList, m_frameReadySemaphore, m_targetSemaphore, m_frameFinishedFence));

   const auto status = m_swapchain.present(m_targetSemaphore, framebufferIndex);
   if (status == Status::OutOfDateSwapchain) {
      this->recreate_swapchain();
   } else {
      GAPI_CHECK_STATUS(status);
   }
}

void SplashScreen::on_close()
{
   sink_OnStartedLoadingAsset.disconnect();

   std::unique_lock lk{m_updateTextMutex};
   m_frameFinishedFence.await();
   m_device.await_all();
}

void SplashScreen::recreate_swapchain()
{
   m_device.await_all();

   const auto newDimension = m_surface.dimension();
   m_resolution = {static_cast<u32>(newDimension.x), static_cast<u32>(newDimension.y)};
   m_swapchain = GAPI_CHECK(m_device.create_swapchain(m_graphicsSurface, GAPI_FORMAT(BGRA, sRGB), ColorSpace::sRGB,
                                                      {static_cast<u32>(newDimension.x), static_cast<u32>(newDimension.y)},
                                                      PresentMode::Fifo, &m_swapchain));
   m_framebuffers = create_framebuffers(m_swapchain, m_renderTarget);
}

void SplashScreen::on_started_loading_asset(const triglav::ResourceName resourceName)
{
   std::unique_lock lk{m_updateTextMutex};
   m_pendingDescChange = fmt::format("Loading resource {}", m_resourceManager.lookup_name(resourceName).value_or("unknown"));
   m_pendingResources.insert(resourceName);
   m_displayedResource = resourceName;
}

void SplashScreen::on_finished_loading_asset(triglav::ResourceName resourceName, triglav::u32 loadedAssets, triglav::u32 totalAssets)
{
   std::unique_lock lk{m_updateTextMutex};
   m_pendingStatusChange.emplace(static_cast<float>(loadedAssets) / static_cast<float>(totalAssets));
   m_pendingResources.erase(resourceName);
   if (m_displayedResource == resourceName && not m_pendingResources.empty()) {
      auto index = rand() % m_pendingResources.size();
      auto it = m_pendingResources.begin();
      std::advance(it, index);
      m_pendingDescChange = fmt::format("Loading resource {}", m_resourceManager.lookup_name(*it).value_or("unknown"));
   }
}

}// namespace demo
