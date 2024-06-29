#pragma once

#include "AmbientOcclusionRenderer.h"
#include "DebugLinesRenderer.h"
#include "GlyphCache.h"
#include "GroundRenderer.h"
#include "InfoDialog.h"
#include "PostProcessingRenderer.h"
#include "RectangleRenderer.h"
#include "Renderer.h"
#include "Scene.h"
#include "ShadingRenderer.h"
#include "SkyBox.h"
#include "SpriteRenderer.h"

#include "triglav/desktop/ISurface.hpp"
#include "triglav/desktop/ISurfaceEventListener.hpp"
#include "triglav/font/FontManager.h"
#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/graphics_api/RenderTarget.h"
#include "triglav/render_core/FrameResources.h"
#include "triglav/render_core/GlyphAtlas.h"
#include "triglav/render_core/Model.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/render_core/RenderGraph.h"
#include "triglav/resource/ResourceManager.h"
#include "triglav/ui_core/Viewport.h"

namespace triglav::renderer {

class Renderer
{
 public:
   enum class Moving
   {
      None,
      Foward,
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
   void on_mouse_wheel_turn(float x);
   [[nodiscard]] resource::ResourceManager& resource_manager() const;
   [[nodiscard]] std::tuple<uint32_t, uint32_t> screen_resolution() const;
   [[nodiscard]] graphics_api::Device& device() const;

 private:
   void recreate_swapchain(uint32_t width, uint32_t height);
   void update_uniform_data(float deltaTime);
   static float calculate_frame_duration();
   glm::vec3 moving_direction();

   bool m_showDebugLines{false};
   bool m_ssaoEnabled{true};
   bool m_fxaaEnabled{true};
   bool m_bloomEnabled{true};
   bool m_hideUI{false};
   bool m_smoothCamera{true};
   glm::vec3 m_position{};
   glm::vec3 m_motion{};
   glm::vec2 m_mouseOffset{};
   Moving m_moveDirection{Moving::None};

   desktop::ISurface& m_desktopSurface;
   graphics_api::Surface& m_surface;
   graphics_api::Device& m_device;

   resource::ResourceManager& m_resourceManager;
   Scene m_scene;
   graphics_api::Resolution m_resolution;
   graphics_api::Swapchain m_swapchain;
   graphics_api::RenderTarget m_renderTarget;
   std::vector<graphics_api::Framebuffer> m_framebuffers;
   GlyphCache m_glyphCache;
   SpriteRenderer m_context2D;
   render_core::RenderGraph m_renderGraph;
   ui_core::Viewport m_uiViewport;
   InfoDialog m_infoDialog;
};

}// namespace triglav::renderer