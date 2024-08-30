#pragma once

#include "SplashScreen.h"

#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/font/FontManager.h"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/renderer/Renderer.h"
#include "triglav/resource/ResourceManager.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace demo {

class GameInstance
{
 public:
   enum class State
   {
      Uninitialized,
      LoadingBaseResources,
      LoadingResources,
      Ready
   };

   GameInstance(triglav::desktop::IDisplay& display, triglav::graphics_api::Resolution&& resolution);

   void on_loaded_assets();
   void loop(triglav::desktop::IDisplay& display);

 private:
   std::shared_ptr<triglav::desktop::ISurface> m_splashScreenSurface;
   std::shared_ptr<triglav::desktop::ISurface> m_demoSurface;
   triglav::graphics_api::Resolution m_resolution;
   triglav::graphics_api::Instance m_instance;
   std::optional<triglav::graphics_api::Surface> m_graphicsSplashScreenSurface;
   std::optional<triglav::graphics_api::Surface> m_graphicsDemoSurface;
   triglav::graphics_api::DeviceUPtr m_device;
   triglav::font::FontManger m_fontManager;
   triglav::resource::ResourceManager m_resourceManager;
   std::unique_ptr<SplashScreen> m_splashScreen;
   std::unique_ptr<triglav::renderer::Renderer> m_renderer;
   std::unique_ptr<triglav::desktop::ISurfaceEventListener> m_eventListener;
   std::mutex m_stateMtx;
   std::atomic<State> m_state{State::Uninitialized};
   std::condition_variable m_baseResourcesReadyCV;

   triglav::resource::ResourceManager::OnLoadedAssetsDel::Sink<GameInstance> m_onLoadedAssetsSink;
};

}// namespace demo