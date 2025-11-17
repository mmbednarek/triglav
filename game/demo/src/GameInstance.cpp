#include "GameInstance.hpp"

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
using namespace triglav::string_literals;

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
   m_mouse_position = position;
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
   m_renderer.on_mouse_is_pressed(button, m_mouse_position);

   if (not m_surface.is_cursor_locked() && button == MouseButton::Middle) {
      m_surface.lock_cursor();
      m_surface.hide_cursor();
   }
}

void EventListener::on_mouse_button_is_released(const MouseButton button) const
{
   m_renderer.on_mouse_is_released(button, m_mouse_position);

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
   m_is_running = false;
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
   return m_is_running;
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
    m_splash_screen_surface(
       display.create_surface("Triglav Engine Demo - Loading"_strv, {1024, 360}, WindowAttribute::AlignCenter | WindowAttribute::TopMost)),
    m_resolution(resolution),
    m_instance(GAPI_CHECK(gapi::Instance::create_instance(&display))),
    m_graphics_splash_screen_surface(GAPI_CHECK(m_instance.create_surface(*m_splash_screen_surface))),
    m_device(
       GAPI_CHECK(m_instance.create_device(&*m_graphics_splash_screen_surface, device_pick_strategy(), requested_features(m_instance)))),
    m_resource_manager(*m_device, m_font_manager),
    TG_CONNECT(m_resource_manager, OnLoadedAssets, on_loaded_assets)
{
   m_state = State::LoadingBaseResources;
   m_resource_manager.load_asset_list(PathManager::the().content_path().sub("index_base.yaml"));
}

void GameInstance::on_loaded_assets()
{
   {
      if (m_state.load() == State::LoadingBaseResources) {
         {
            std::unique_lock lk{m_state_mtx};
            m_state.store(State::LoadingResources);
         }
         m_base_resources_ready_cv.notify_one();
         m_resource_manager.load_asset_list(PathManager::the().content_path().sub("index.yaml"));
      } else {
         m_state.store(State::Ready);
      }
   }
}

void GameInstance::loop(triglav::desktop::IDisplay& display)
{
   display.dispatch_messages();

   std::unique_lock lk{m_state_mtx};
   m_base_resources_ready_cv.wait(lk, [this] { return m_state == State::LoadingResources; });
   lk.unlock();

   m_splash_screen =
      std::make_unique<SplashScreen>(*m_splash_screen_surface, *m_graphics_splash_screen_surface, *m_device, m_resource_manager);

   while (m_state.load() != State::Ready) {
      m_splash_screen->update();
      display.dispatch_messages();
   }

   m_splash_screen->on_close();
   m_splash_screen.reset();
   m_device->await_all();

   m_graphics_splash_screen_surface.reset();
   m_splash_screen_surface.reset();

   display.dispatch_messages();

   m_demo_surface = display.create_surface("Triglav Engine Demo"_strv, {m_resolution.width, m_resolution.height}, WindowAttribute::Default);
   m_graphics_demo_surface.emplace(GAPI_CHECK(m_instance.create_surface(*m_demo_surface)));

   m_renderer =
      std::make_unique<triglav::renderer::Renderer>(*m_demo_surface, *m_graphics_demo_surface, *m_device, m_resource_manager, m_resolution);
   m_event_listener.emplace(*m_demo_surface, *m_renderer);

   auto& event_listener = dynamic_cast<EventListener&>(*m_event_listener);

   while (event_listener.is_running()) {
      m_renderer->on_render();
      display.dispatch_messages();
   }

   m_renderer->on_close();
}

}// namespace demo
