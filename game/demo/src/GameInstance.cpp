#include "GameInstance.h"

#include "triglav/resource/PathManager.h"

namespace demo {

using triglav::desktop::DefaultSurfaceEventListener;
using triglav::desktop::ISurface;
using triglav::desktop::Key;
using triglav::desktop::MouseButton;
using triglav::resource::PathManager;

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

GameInstance::GameInstance(triglav::desktop::ISurface& surface, triglav::graphics_api::Resolution&& resolution) :
    m_surface(surface),
    m_resolution(resolution),
    m_device(GAPI_CHECK(triglav::graphics_api::initialize_device(surface))),
    m_resourceManager(*m_device, m_fontManager),
    m_onLoadedAssetsSink(m_resourceManager.OnLoadedAssets.connect<&GameInstance::on_loaded_assets>(this))
{
   m_resourceManager.load_asset_list(PathManager::the().content_path().sub("index.yaml"));
}

void GameInstance::on_loaded_assets()
{
   std::unique_lock lk{m_assetsAreLoadedMtx};
   m_renderer = std::make_unique<triglav::renderer::Renderer>(*m_device, m_resourceManager, m_resolution);
   m_eventListener = std::make_unique<EventListener>(m_surface, *m_renderer);
   m_surface.add_event_listener(m_eventListener.get());

   lk.unlock();
   m_assetsAreLoadedCV.notify_one();
}

void GameInstance::loop(triglav::desktop::IDisplay& display)
{
   display.dispatch_messages();

   std::unique_lock lk{m_assetsAreLoadedMtx};
   m_assetsAreLoadedCV.wait(lk, [this] { return m_renderer != nullptr && m_eventListener != nullptr; });

   lk.unlock();

   auto& eventListener = dynamic_cast<EventListener&>(*m_eventListener);

   while (eventListener.is_running()) {
      m_renderer->on_render();
      display.dispatch_messages();
   }

   m_renderer->on_close();
}

}// namespace demo
