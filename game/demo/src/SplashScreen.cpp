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
    m_swapchain(
       GAPI_CHECK(m_device.create_swapchain(m_graphicsSurface, GAPI_FORMAT(BGRA, sRGB), ColorSpace::sRGB, g_splashScreenResolution))),
    m_renderTarget(GAPI_CHECK(RenderTargetBuilder(m_device)
                                 .attachment("output"_name,
                                             AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                AttachmentAttribute::StoreImage | AttachmentAttribute::Presentable,
                                             g_splashScreenColorFormat)
                                 .build())),
    m_framebuffers(create_framebuffers(m_swapchain, m_renderTarget)),
    m_textRenderer(m_device, m_resourceManager, m_renderTarget),
    m_commandList(GAPI_CHECK(m_device.queue_manager().create_command_list(WorkType::Graphics))),
    m_frameReadySemaphore(GAPI_CHECK(m_device.create_semaphore())),
    m_targetSemaphore(GAPI_CHECK(m_device.create_semaphore())),
    m_frameFinishedFence(GAPI_CHECK(m_device.create_fence())),
    m_textTitle(m_textRenderer.create_text_object("cantarell/bold.glyphs"_rc, "TRIGLAV RENDER DEMO")),
    m_textDesc(m_textRenderer.create_text_object("cantarell.glyphs"_rc, "Loading Resource")),
    m_textStatus(m_textRenderer.create_text_object("cantarell.glyphs"_rc, "[0/?]")),
    m_onStartedLoadingAssetSink(m_resourceManager.OnStartedLoadingAsset.connect<&SplashScreen::on_started_loading_asset>(this)),
    m_onFinishedLoadingAssetSink(m_resourceManager.OnFinishedLoadingAsset.connect<&SplashScreen::on_finished_loading_asset>(this))
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

   m_textRenderer.bind_pipeline(m_commandList);
   m_textRenderer.draw_text(m_commandList, m_textTitle, {g_splashScreenResolution.width, g_splashScreenResolution.height}, {64.0f, 64.0f},
                            {0.13f, 0.39f, 0.78f, 1.0f});

   {
      std::unique_lock lk{m_updateTextMutex};
      m_textRenderer.draw_text(m_commandList, m_textDesc, {g_splashScreenResolution.width, g_splashScreenResolution.height},
                               {64.0f, 128.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text(m_commandList, m_textStatus, {g_splashScreenResolution.width, g_splashScreenResolution.height},
                               {64.0f, 160.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
   }

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
   m_onStartedLoadingAssetSink.disconnect();

   std::unique_lock lk{m_updateTextMutex};
   m_frameFinishedFence.await();
   m_device.await_all();
}

void SplashScreen::recreate_swapchain()
{
   m_device.await_all();

   const auto newDimension = m_surface.dimension();
   m_swapchain =
      GAPI_CHECK(m_device.create_swapchain(m_graphicsSurface, GAPI_FORMAT(BGRA, sRGB), ColorSpace::sRGB,
                                           {static_cast<u32>(newDimension.width), static_cast<u32>(newDimension.height)}, &m_swapchain));
   m_framebuffers = create_framebuffers(m_swapchain, m_renderTarget);
}

void SplashScreen::on_started_loading_asset(const triglav::ResourceName resourceName)
{
   std::unique_lock lk{m_updateTextMutex};

   auto msg = fmt::format("Loading resource {}...", m_resourceManager.lookup_name(resourceName).value_or("unknown"));
   m_textRenderer.update_text(m_textDesc, msg);
}

void SplashScreen::on_finished_loading_asset(triglav::ResourceName resourceName, triglav::u32 loadedAssets, triglav::u32 totalAssets)
{
   std::unique_lock lk{m_updateTextMutex};

   auto msg = fmt::format("[{}/{}]", loadedAssets, totalAssets);
   m_textRenderer.update_text(m_textStatus, msg);
}

}// namespace demo
