#pragma once

#include "AmbientOcclusionRenderer.hpp"
#include "BindlessScene.hpp"
#include "GlyphCache.hpp"
#include "InfoDialog.hpp"
#include "RayTracingScene.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "SpriteRenderer.hpp"

#include "triglav/desktop/ISurfaceEventListener.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/RenderTarget.hpp"
#include "triglav/render_core/GlyphAtlas.hpp"
#include "triglav/render_core/RenderGraph.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <optional>

namespace triglav::renderer {

class Renderer
{
 public:
   enum class Moving
   {
      None,
      Forward,
      Backwards,
      Left,
      Right,
      Up,
      Down
   };

   Renderer(desktop::ISurface& desktopSurface, graphics_api::Surface& surface, graphics_api::Device& device,
            resource::ResourceManager& resourceManager, const graphics_api::Resolution& resolution);

   void update_debug_info();
   void on_render();
   void on_resize(uint32_t width, uint32_t height);
   void on_close();
   void on_mouse_relative_move(float dx, float dy);
   void on_key_pressed(desktop::Key key);
   void on_key_released(desktop::Key key);
   [[nodiscard]] resource::ResourceManager& resource_manager() const;
   [[nodiscard]] std::tuple<uint32_t, uint32_t> screen_resolution() const;
   [[nodiscard]] graphics_api::Device& device() const;
   [[nodiscard]] bool is_any_ray_tracing_feature_enabled() const;

 private:
   void recreate_swapchain(uint32_t width, uint32_t height);
   void update_uniform_data(float deltaTime);
   static float calculate_frame_duration();
   glm::vec3 moving_direction();

   bool m_showDebugLines{false};
   AmbientOcclusionMethod m_aoMethod{AmbientOcclusionMethod::ScreenSpace};
   bool m_fxaaEnabled{true};
   bool m_bloomEnabled{true};
   bool m_hideUI{false};
   bool m_smoothCamera{true};
   bool m_mustRecreateSwapchain{false};
   bool m_rayTracedShadows{false};
   glm::vec3 m_position{};
   glm::vec3 m_motion{};
   glm::vec2 m_mouseOffset{};
   Moving m_moveDirection{Moving::None};

   desktop::ISurface& m_desktopSurface;
   graphics_api::Surface& m_surface;
   graphics_api::Device& m_device;

   resource::ResourceManager& m_resourceManager;
   Scene m_scene;
   BindlessScene m_bindlessScene;
   graphics_api::Resolution m_resolution;
   graphics_api::Swapchain m_swapchain;
   graphics_api::RenderTarget m_renderTarget;
   std::vector<graphics_api::Framebuffer> m_framebuffers;
   GlyphCache m_glyphCache;
   SpriteRenderer m_context2D;
   render_core::RenderGraph m_renderGraph;
   ui_core::Viewport m_uiViewport;
   InfoDialog m_infoDialog;
   std::optional<RayTracingScene> m_rayTracingScene;
};

}// namespace triglav::renderer