#include "renderer/Renderer.h"
#include "wayland/Display.h"
#include "wayland/Surface.h"
#include "object_reader/Reader.h"

#include <fstream>

constexpr auto g_initialWidth  = 1280;
constexpr auto g_initialHeight = 720;

class EventListener final : public wayland::DefaultSurfaceEventListener
{
 public:
   EventListener(wayland::Surface &surface, renderer::Renderer &renderer) :
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

   void on_mouse_button_is_pressed(const wayland::MouseButton button) override
   {
      if (not m_surface.is_cursor_locked() && button == wayland::MouseButton_Middle) {
         m_surface.lock_cursor();
      }
   }

   void on_mouse_button_is_released(const wayland::MouseButton button) override
   {
      if (m_surface.is_cursor_locked() && button == wayland::MouseButton_Middle) {
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

   [[nodiscard]] bool is_running() const
   {
      return m_isRunning;
   }

 private:
   wayland::Surface &m_surface;
   renderer::Renderer &m_renderer;
   bool m_isRunning{true};
};

int main()
{
   wayland::Display display;
   wayland::Surface surface(display);

   auto renderer = renderer::init_renderer(surface.to_grahics_surface(), g_initialWidth, g_initialHeight);


   EventListener eventListener(surface, renderer);
   surface.set_event_listener(&eventListener);

   while (eventListener.is_running()) {
      display.dispatch();
      renderer.on_render();
   }

   renderer.on_close();
}