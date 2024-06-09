#pragma once

#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/font/FontManager.h"
#include "triglav/graphics_api/Device.h"
#include "triglav/renderer/Renderer.h"
#include "triglav/resource/ResourceManager.h"

#include <condition_variable>
#include <memory>
#include <mutex>

namespace demo {

class GameInstance
{
 public:
   GameInstance(triglav::desktop::ISurface& surface, triglav::graphics_api::Resolution&& resolution);

   void on_loaded_assets();
   void loop(triglav::desktop::IDisplay& display);

 private:
   triglav::desktop::ISurface& m_surface;
   triglav::graphics_api::Resolution m_resolution;
   triglav::graphics_api::DeviceUPtr m_device;
   triglav::font::FontManger m_fontManager;
   triglav::resource::ResourceManager m_resourceManager;
   std::unique_ptr<triglav::renderer::Renderer> m_renderer;
   std::unique_ptr<triglav::desktop::ISurfaceEventListener> m_eventListener;
   std::mutex m_assetsAreLoadedMtx;
   std::condition_variable m_assetsAreLoadedCV;

   triglav::resource::ResourceManager::OnLoadedAssetsDel::Sink<GameInstance> m_onLoadedAssetsSink;
};

}// namespace demo