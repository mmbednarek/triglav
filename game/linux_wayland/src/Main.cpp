#include "renderer/Renderer.h"
#include "wayland/Display.h"
#include "wayland/Surface.h"

constexpr auto g_initialWidth  = 1280;
constexpr auto g_initialHeight = 720;

int main()
{
   const wayland::Display display;
   wayland::Surface surface(display);

   auto renderer = renderer::init_renderer(surface.to_grahics_surface(), g_initialWidth, g_initialHeight);

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