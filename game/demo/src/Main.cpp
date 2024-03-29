#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/desktop/ISurfaceEventListener.hpp"
#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/renderer/Renderer.h"

#include <iostream>

using triglav::desktop::DefaultSurfaceEventListener;
using triglav::desktop::IDisplay;
using triglav::desktop::ISurface;
using triglav::desktop::MouseButton;
using triglav::desktop::InputArgs;

constexpr auto g_initialWidth  = 1280;
constexpr auto g_initialHeight = 720;

class EventListener final : public DefaultSurfaceEventListener
{
 public:
   EventListener(ISurface &surface, triglav::renderer::Renderer &renderer) :
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

   void on_key_is_pressed(const triglav::desktop::Key key) override
   {
      // W - 17
      // A - 30
      // S - 31
      // S - 32
      // std::cout << "pressed key: " << key << '\n';
      m_renderer.on_key_pressed(key);
   }

   void on_key_is_released(const triglav::desktop::Key key) override
   {
      // std::cout << "released key: " << key << '\n';
      m_renderer.on_key_released(key);
   }

   [[nodiscard]] bool is_running() const
   {
      return m_isRunning;
   }

 private:
   ISurface &m_surface;
   triglav::renderer::Renderer &m_renderer;
   bool m_isRunning{true};
};

int triglav_main(InputArgs& /*args*/, IDisplay& display)
{
   const auto surface = display.create_surface(g_initialWidth, g_initialHeight);

   triglav::renderer::Renderer renderer(*surface, g_initialWidth, g_initialHeight);

   EventListener eventListener(*surface, renderer);
   surface->add_event_listener(&eventListener);

   while (eventListener.is_running()) {
      display.dispatch_messages();
      renderer.on_render();
   }

   renderer.on_close();

   return EXIT_SUCCESS;
}
