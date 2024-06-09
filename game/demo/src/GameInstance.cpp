#include "GameInstance.h"

#include "triglav/resource/PathManager.h"

namespace demo {

using triglav::desktop::DefaultSurfaceEventListener;
using triglav::desktop::ISurface;
using triglav::desktop::Key;
using triglav::desktop::MouseButton;
using triglav::resource::PathManager;
namespace gapi = triglav::graphics_api;

class EventListener final : public DefaultSurfaceEventListener
{
 public:
   EventListener(ISurface& surface, triglav::renderer::Renderer& renderer) :
       m_surface(surface),
       m_renderer(renderer)
   {
   }

   void on_mouse_relative_move(const float dx, const float dy) override
   {
      if (not m_surface.is_cursor_locked())
         return;

      m_renderer.on_mouse_relative_move(dx, dy);
   }

   void on_mouse_leave() override
   {
      m_surface.unlock_cursor();
   }

   void on_mouse_wheel_turn(const float x) override
   {
      m_renderer.on_mouse_wheel_turn(x);
   }

   void on_mouse_button_is_pressed(const MouseButton button) override
   {
      if (not m_surface.is_cursor_locked() && button == MouseButton::Middle) {
         m_surface.lock_cursor();
         m_surface.hide_cursor();
      }
   }

   void on_mouse_button_is_released(const MouseButton button) override
   {
      if (m_surface.is_cursor_locked() && button == MouseButton::Middle) {
         m_surface.unlock_cursor();
      }
   }

   void on_resize(const int width, const int height) override
   {
      m_renderer.on_resize(width, height);
   }

   void on_close() override
   {
      m_isRunning = false;
   }

   void on_key_is_pressed(const Key key) override
   {
      // W - 17
      // A - 30
      // S - 31
      // S - 32
      // std::cout << "pressed key: " << key << '\n';
      m_renderer.on_key_pressed(key);
   }

   void on_key_is_released(const Key key) override
   {
      // std::cout << "released key: " << key << '\n';
      m_renderer.on_key_released(key);
   }

   [[nodiscard]] bool is_running() const
   {
      return m_isRunning;
   }

 private:
   ISurface& m_surface;
   triglav::renderer::Renderer& m_renderer;
   bool m_isRunning{true};
};

GameInstance::GameInstance(triglav::desktop::IDisplay& display, triglav::graphics_api::Resolution&& resolution) :
    m_splashScreenSurface(display.create_surface(1024, 360)),
    m_resolution(resolution),
    m_instance(GAPI_CHECK(gapi::Instance::create_instance())),
    m_graphicsSplashScreenSurface(GAPI_CHECK(m_instance.create_surface(*m_splashScreenSurface))),
    m_device(GAPI_CHECK(m_instance.create_device(*m_graphicsSplashScreenSurface))),
    m_resourceManager(*m_device, m_fontManager),
    m_onLoadedAssetsSink(m_resourceManager.OnLoadedAssets.connect<&GameInstance::on_loaded_assets>(this))
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

   m_splashScreen = std::make_unique<SplashScreen>(*m_graphicsSplashScreenSurface, *m_device, m_resourceManager);

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

   m_demoSurface = display.create_surface(static_cast<int>(m_resolution.width), static_cast<int>(m_resolution.height));
   m_graphicsDemoSurface.emplace(GAPI_CHECK(m_instance.create_surface(*m_demoSurface)));

   m_renderer = std::make_unique<triglav::renderer::Renderer>(*m_graphicsDemoSurface, *m_device, m_resourceManager, m_resolution);
   m_eventListener = std::make_unique<EventListener>(*m_demoSurface, *m_renderer);
   m_demoSurface->add_event_listener(m_eventListener.get());

   auto& eventListener = dynamic_cast<EventListener&>(*m_eventListener);

   while (eventListener.is_running()) {
      m_renderer->on_render();
      display.dispatch_messages();
   }

   m_renderer->on_close();
}

}// namespace demo
