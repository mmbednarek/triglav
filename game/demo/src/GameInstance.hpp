#pragma once

#include "SplashScreen.hpp"

#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/font/FontManager.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Instance.hpp"
#include "triglav/renderer/Renderer.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace demo {

class EventListener final
{
 public:
   using Self = EventListener;

   EventListener(triglav::desktop::ISurface& surface, triglav::renderer::Renderer& renderer);

   void on_mouse_move(triglav::Vector2 position);
   void on_mouse_relative_move(triglav::Vector2 offset) const;
   void on_mouse_leave() const;
   void on_mouse_button_is_pressed(triglav::desktop::MouseButton button) const;
   void on_mouse_button_is_released(triglav::desktop::MouseButton button) const;
   void on_resize(triglav::Vector2i resolution) const;
   void on_close();
   void on_key_is_pressed(triglav::desktop::Key key) const;
   void on_key_is_released(triglav::desktop::Key key) const;

   [[nodiscard]] bool is_running() const;

 private:
   triglav::desktop::ISurface& m_surface;
   triglav::renderer::Renderer& m_renderer;
   bool m_is_running{true};
   triglav::Vector2 m_mouse_position;

   TG_SINK(triglav::desktop::ISurface, OnMouseMove);
   TG_SINK(triglav::desktop::ISurface, OnMouseRelativeMove);
   TG_SINK(triglav::desktop::ISurface, OnMouseLeave);
   TG_SINK(triglav::desktop::ISurface, OnMouseButtonIsPressed);
   TG_SINK(triglav::desktop::ISurface, OnMouseButtonIsReleased);
   TG_SINK(triglav::desktop::ISurface, OnResize);
   TG_SINK(triglav::desktop::ISurface, OnClose);
   TG_SINK(triglav::desktop::ISurface, OnKeyIsPressed);
   TG_SINK(triglav::desktop::ISurface, OnKeyIsReleased);
};

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

   using Self = GameInstance;

   GameInstance(triglav::desktop::IDisplay& display, triglav::graphics_api::Resolution&& resolution);

   void on_loaded_assets();
   void loop(triglav::desktop::IDisplay& display);

 private:
   std::shared_ptr<triglav::desktop::ISurface> m_splash_screen_surface;
   std::shared_ptr<triglav::desktop::ISurface> m_demo_surface;
   triglav::graphics_api::Resolution m_resolution;
   triglav::graphics_api::Instance m_instance;
   std::optional<triglav::graphics_api::Surface> m_graphics_splash_screen_surface;
   std::optional<triglav::graphics_api::Surface> m_graphics_demo_surface;
   triglav::graphics_api::DeviceUPtr m_device;
   triglav::font::FontManger m_font_manager;
   triglav::resource::ResourceManager m_resource_manager;
   std::unique_ptr<SplashScreen> m_splash_screen;
   std::unique_ptr<triglav::renderer::Renderer> m_renderer;
   std::optional<EventListener> m_event_listener;
   std::mutex m_state_mtx;
   std::atomic<State> m_state{State::Uninitialized};
   std::condition_variable m_base_resources_ready_cv;

   TG_SINK(triglav::resource::ResourceManager, OnLoadedAssets);
};

}// namespace demo