#include "renderer/Renderer.h"
#include "wayland/Display.h"
#include "wayland/Surface.h"

#include <iostream>

constexpr auto g_initialWidth  = 1280;
constexpr auto g_initialHeight = 720;

class EventListener final : public wayland::IDisplayEventListener
{
 public:
   void on_pointer_enter_surface(wayland::Surface &surface, float pos_x, float pos_y) override
   {
      // std::cout << "pointer enter\n";
      m_pointerSurface = &surface;
      // surface.lock_cursor();
   }

   void on_pointer_change_position(const float /*pos_x*/, const float /*pos_y*/) override {}

   void on_pointer_relative_motion(const float dx, const float dy) override
   {
      if (m_renderer == nullptr)
         return;
      if (m_pointerSurface == nullptr)
         return;
      if (not m_pointerSurface->is_cursor_locked())
         return;
      m_renderer->on_mouse_relative_move(dx, dy);
   }

   void on_pointer_leave_surface(wayland::Surface &surface) override
   {
      surface.unlock_cursor();
      m_pointerSurface = nullptr;
   }

   void on_mouse_wheel_turn(const float x) override
   {
      if (m_renderer == nullptr)
         return;
      if (m_pointerSurface == nullptr)
         return;
      if (not m_pointerSurface->is_cursor_locked())
         return;
      m_renderer->on_mouse_wheel_turn(x);
   }

   void on_mouse_button_is_pressed(const uint32_t button) override
   {
      if (button != 274)
         return;
      if (m_pointerSurface == nullptr)
         return;

      if (m_pointerSurface->is_cursor_locked()) {
         m_pointerSurface->unlock_cursor();
      } else {
         m_pointerSurface->lock_cursor();
         m_waylandDisplay->hide_pointer();
      }
   }

   void set_renderer(renderer::Renderer *renderer)
   {
      m_renderer = renderer;
   }

   void set_display(wayland::Display *display)
   {
      m_waylandDisplay = display;
   }

 private:
   renderer::Renderer *m_renderer{};
   wayland::Display *m_waylandDisplay{};
   wayland::Surface *m_pointerSurface{};
};

int main()
{
   EventListener eventListener;
   wayland::Display display(eventListener);
   wayland::Surface surface(display);

   auto renderer = renderer::init_renderer(surface.to_grahics_surface(), g_initialWidth, g_initialHeight);
   eventListener.set_renderer(&renderer);
   eventListener.set_display(&display);

   bool isRunning{true};
   while (isRunning) {
      switch (surface.message()) {
      case wayland::SurfaceMessage::None: display.dispatch(); break;
      case wayland::SurfaceMessage::Resize: {
         const auto [width, height] = surface.dimension();
         renderer.on_resize(width, height);
         break;
      }
      case wayland::SurfaceMessage::Close: isRunning = false; break;
      }

      renderer.on_render();
   }

   renderer.on_close();
}