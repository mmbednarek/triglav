#include "GameInstance.h"

#include "triglav/io/CommandLine.hpp"
#include "triglav/resource/PathManager.hpp"

namespace demo {

using triglav::desktop::ISurface;
using triglav::desktop::Key;
using triglav::desktop::MouseButton;
using triglav::desktop::WindowAttribute;
using triglav::resource::PathManager;
namespace gapi = triglav::graphics_api;

using namespace triglav::name_literals;

EventListener::EventListener(ISurface& surface, triglav::renderer::Renderer& renderer) :
    m_surface(surface),
    m_renderer(renderer),
    TG_CONNECT(surface, OnMouseMove, on_mouse_move),
    TG_CONNECT(surface, OnMouseRelativeMove, on_mouse_relative_move),
    TG_CONNECT(surface, OnMouseLeave, on_mouse_leave),
    TG_CONNECT(surface, OnMouseButtonIsPressed, on_mouse_button_is_pressed),
    TG_CONNECT(surface, OnMouseButtonIsReleased, on_mouse_button_is_released),
    TG_CONNECT(surface, OnResize, on_resize),
    TG_CONNECT(surface, OnClose, on_close),
    TG_CONNECT(surface, OnKeyIsPressed, on_key_is_pressed),
    TG_CONNECT(surface, OnKeyIsReleased, on_key_is_released)
{
}

void EventListener::on_mouse_move(const triglav::Vector2 position)
{
   m_mousePosition = position;
   m_renderer.on_mouse_move(position);
}

void EventListener::on_mouse_relative_move(const triglav::Vector2 offset) const
{
   if (not m_surface.is_cursor_locked())
      return;

   m_renderer.on_mouse_relative_move(offset.x, offset.y);
}

void EventListener::on_mouse_leave() const
{
   m_surface.unlock_cursor();
}

void EventListener::on_mouse_button_is_pressed(const MouseButton button) const
{
   m_renderer.on_mouse_is_pressed(button, m_mousePosition);

   if (not m_surface.is_cursor_locked() && button == MouseButton::Middle) {
      m_surface.lock_cursor();
      m_surface.hide_cursor();
   }
}

void EventListener::on_mouse_button_is_released(const MouseButton button) const
{
   m_renderer.on_mouse_is_released(button, m_mousePosition);

   if (m_surface.is_cursor_locked() && button == MouseButton::Middle) {
      m_surface.unlock_cursor();
   }
}

void EventListener::on_resize(const triglav::Vector2i resolution) const
{
   m_renderer.on_resize(resolution.x, resolution.y);
}

void EventListener::on_close()
{
   m_isRunning = false;
}

void EventListener::on_key_is_pressed(const Key key) const
{
   // W - 17
   // A - 30
   // S - 31
   // S - 32
   // std::cout << "pressed key: " << key << '\n';
   m_renderer.on_key_pressed(key);
}

void EventListener::on_key_is_released(const Key key) const
{
   // std::cout << "released key: " << key << '\n';
   m_renderer.on_key_released(key);
}

[[nodiscard]] bool EventListener::is_running() const
{
   return m_isRunning;
}

namespace {

gapi::DevicePickStrategy device_pick_strategy()
{
   if (triglav::io::CommandLine::the().is_enabled("preferIntegratedGpu"_name))
      return gapi::DevicePickStrategy::PreferIntegrated;

   return gapi::DevicePickStrategy::PreferDedicated;
}

gapi::DeviceFeatureFlags requested_features(const gapi::Instance& instance)
{
   if (triglav::io::CommandLine::the().is_enabled("noRayTracing"_name))
      return {};

   if (instance.are_features_supported(gapi::DeviceFeature::RayTracing)) {
      return gapi::DeviceFeature::RayTracing;
   }

   return {};
}

}// namespace

GameInstance::GameInstance(triglav::desktop::IDisplay& display, triglav::graphics_api::Resolution&& resolution) :
    m_splashScreenSurface(
       display.create_surface("Triglav Engine Demo - Loading", {1024, 360}, WindowAttribute::AlignCenter | WindowAttribute::TopMost)),
    m_resolution(resolution),
    m_instance(GAPI_CHECK(gapi::Instance::create_instance())),
    m_graphicsSplashScreenSurface(GAPI_CHECK(m_instance.create_surface(*m_splashScreenSurface))),
    m_device(GAPI_CHECK(m_instance.create_device(&*m_graphicsSplashScreenSurface, device_pick_strategy(), requested_features(m_instance)))),
    m_resourceManager(*m_device, m_fontManager),
    TG_CONNECT(m_resourceManager, OnLoadedAssets, on_loaded_assets)
{
   m_state = State::LoadingBaseResources;
   m_resourceManager.load_asset_list(PathManager::the().content_path().sub("index_base.yaml"));
}

void GameInstance::on_loaded_assets()
{
   {
      if (m_state.load() == State::LoadingBaseResources) {
         {
            std::unique_lock lk{m_stateMtx};
            m_state.store(State::LoadingResources);
         }
         m_baseResourcesReadyCV.notify_one();
         m_resourceManager.load_asset_list(PathManager::the().content_path().sub("index.yaml"));
      } else {
         m_state.store(State::Ready);
      }
   }
}

void GameInstance::loop(triglav::desktop::IDisplay& display)
{
   display.dispatch_messages();

   std::unique_lock lk{m_stateMtx};
   m_baseResourcesReadyCV.wait(lk, [this] { return m_state == State::LoadingResources; });
   lk.unlock();

   m_splashScreen = std::make_unique<SplashScreen>(*m_splashScreenSurface, *m_graphicsSplashScreenSurface, *m_device, m_resourceManager);

   while (m_state.load() != State::Ready) {
      m_splashScreen->update();
      display.dispatch_messages();
   }

   m_splashScreen->on_close();
   m_splashScreen.reset();
   m_device->await_all();

   m_graphicsSplashScreenSurface.reset();
   m_splashScreenSurface.reset();

   display.dispatch_messages();

   m_demoSurface = display.create_surface("Triglav Engine Demo", {m_resolution.width, m_resolution.height}, WindowAttribute::Default);
   m_graphicsDemoSurface.emplace(GAPI_CHECK(m_instance.create_surface(*m_demoSurface)));

   m_renderer =
      std::make_unique<triglav::renderer::Renderer>(*m_demoSurface, *m_graphicsDemoSurface, *m_device, m_resourceManager, m_resolution);
   m_eventListener.emplace(*m_demoSurface, *m_renderer);

   auto& eventListener = dynamic_cast<EventListener&>(*m_eventListener);

   while (eventListener.is_running()) {
      m_renderer->on_render();
      display.dispatch_messages();
   }

   m_renderer->on_close();
}

}// namespace demo
