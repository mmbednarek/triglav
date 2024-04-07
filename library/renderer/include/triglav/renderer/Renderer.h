#pragma once

#include "AmbientOcclusionRenderer.h"
#include "DebugLinesRenderer.h"
#include "GroundRenderer.h"
#include "ModelRenderer.h"
#include "PostProcessingRenderer.h"
#include "RectangleRenderer.h"
#include "Renderer.h"
#include "Scene.h"
#include "ShadingRenderer.h"
#include "ShadowMapRenderer.h"
#include "SkyBox.h"
#include "SpriteRenderer.h"

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

   Renderer(const desktop::ISurface &surface, uint32_t width, uint32_t height);
   void update_debug_info(float framerate);
   void on_render();
   void on_resize(uint32_t width, uint32_t height);
   void on_close() const;
   void on_mouse_relative_move(float dx, float dy);
   void on_key_pressed(triglav::desktop::Key key);
   void on_key_released(triglav::desktop::Key key);
   void on_mouse_wheel_turn(float x);
   [[nodiscard]] graphics_api::PipelineBuilder create_pipeline();
   [[nodiscard]] triglav::resource::ResourceManager &resource_manager() const;
   [[nodiscard]] std::tuple<uint32_t, uint32_t> screen_resolution() const;

   template<typename TUbo>
   [[nodiscard]] graphics_api::Buffer create_ubo_buffer() const
   {
      return triglav::render_core::checkResult(
              m_device->create_buffer(graphics_api::BufferPurpose::UniformBuffer, sizeof(TUbo)));
   }

   [[nodiscard]] graphics_api::Device &device() const;

 private:
   void update_uniform_data(float deltaTime);
   static float calculate_frame_duration();
   static float calculate_framerate(float frameDuration);
   glm::vec3 moving_direction() const;

   bool m_receivedMouseInput{false};
   float m_lastMouseX{};
   float m_lastMouseY{};

   float m_distance{12};
   float m_lightX{-40};
   bool m_showDebugLines{false};
   bool m_ssaoEnabled{true};
   bool m_fxaaEnabled{true};
   glm::vec3 m_position{};
   glm::vec3 m_motion{};
   Moving m_moveDirection{Moving::None};

   graphics_api::DeviceUPtr m_device;

   font::FontManger m_fontManger;
   std::unique_ptr<resource::ResourceManager> m_resourceManager;

   graphics_api::Resolution m_resolution;
   graphics_api::Swapchain m_swapchain;
   graphics_api::RenderTarget m_renderTarget;
   std::vector<graphics_api::Framebuffer> m_framebuffers;
   graphics_api::Semaphore m_framebufferReadySemaphore;
   graphics_api::RenderTarget m_geometryRenderTarget;
   graphics_api::Framebuffer m_geometryBuffer;
   graphics_api::RenderTarget m_shadingRenderTarget;
   graphics_api::Framebuffer m_shadingFramebuffer;
   ModelRenderer m_modelRenderer;
   GroundRenderer m_groundRenderer;
   SpriteRenderer m_context2D;
   ShadowMapRenderer m_shadowMapRenderer;
   DebugLinesRenderer m_debugLinesRenderer;
   ShadingRenderer m_shadingRenderer;
   PostProcessingRenderer m_postProcessingRenderer;
   Scene m_scene;
   SkyBox m_skyBox;
   render_core::Sprite m_sprite;
   render_core::RenderGraph m_renderGraph;
};

}// namespace triglav::renderer