#pragma once

#include "AmbientOcclusionRenderer.h"
#include "Core.h"
#include "DebugLinesRenderer.h"
#include "GlyphAtlas.h"
#include "GroundRenderer.h"
#include "ModelRenderer.h"
#include "PostProcessingRenderer.h"
#include "RectangleRenderer.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "ShadingRenderer.h"
#include "ShadowMap.h"
#include "SkyBox.h"
#include "SpriteRenderer.h"
#include "TextRenderer.h"

#include "font/FontManager.h"
#include "graphics_api/Device.h"
#include "graphics_api/PipelineBuilder.h"
#include "triglav/desktop/ISurfaceEventListener.hpp"
#include "triglav/resource/ResourceManager.h"

namespace renderer {

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

   Renderer(const triglav::desktop::ISurface &surface, uint32_t width, uint32_t height);
   void on_render();
   void on_resize(uint32_t width, uint32_t height);
   void on_close() const;
   void on_mouse_relative_move(float dx, float dy);
   void on_key_pressed(triglav::desktop::Key key);
   void on_key_released(triglav::desktop::Key key);
   void on_mouse_wheel_turn(float x);
   [[nodiscard]] graphics_api::PipelineBuilder create_pipeline();
   [[nodiscard]] ResourceManager &resource_manager() const;
   [[nodiscard]] std::tuple<uint32_t, uint32_t> screen_resolution() const;

   template<typename TUbo>
   [[nodiscard]] graphics_api::Buffer create_ubo_buffer() const
   {
      return checkResult(m_device->create_buffer(graphics_api::BufferPurpose::UniformBuffer, sizeof(TUbo)));
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

   float m_yaw{0};
   float m_pitch{0};
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
   triglav::resource::ResourceManager m_newResourceManager;
   std::unique_ptr<ResourceManager> m_resourceManager;

   graphics_api::Resolution m_resolution;
   graphics_api::Swapchain m_swapchain;
   graphics_api::RenderPass m_renderPass;
   std::vector<graphics_api::Framebuffer> m_framebuffers;
   graphics_api::Semaphore m_framebufferReadySemaphore;
   graphics_api::Semaphore m_renderFinishedSemaphore;
   graphics_api::Fence m_inFlightFence;
   graphics_api::CommandList m_commandList;
   graphics_api::TextureRenderTarget m_modelRenderTarget;
   graphics_api::RenderPass m_modelRenderPass;
   graphics_api::Texture m_modelColorTexture;
   graphics_api::Texture m_modelPositionTexture;
   graphics_api::Texture m_modelNormalTexture;
   graphics_api::Texture m_modelDepthTexture;
   graphics_api::Framebuffer m_modelFramebuffer;
   graphics_api::TextureRenderTarget m_ambientOcclusionRenderTarget;
   graphics_api::RenderPass m_ambientOcclusionRenderPass;
   graphics_api::Texture m_ambientOcclusionTexture;
   graphics_api::Framebuffer m_ambientOcclusionFramebuffer;
   graphics_api::TextureRenderTarget m_shadingRenderTarget;
   graphics_api::RenderPass m_shadingRenderPass;
   graphics_api::Texture m_shadingColorTexture;
   graphics_api::Framebuffer m_shadingFramebuffer;
   ModelRenderer m_context3D;
   GroundRenderer m_groundRenderer;
   SpriteRenderer m_context2D;
   ShadowMap m_shadowMap;
   DebugLinesRenderer m_debugLinesRenderer;
   RectangleRenderer m_rectangleRenderer;
   Rectangle m_rectangle;
   AmbientOcclusionRenderer m_ambientOcclusionRenderer;
   ShadingRenderer m_shadingRenderer;
   PostProcessingRenderer m_postProcessingRenderer;
   Scene m_scene;
   SkyBox m_skyBox;
   GlyphAtlas m_glyphAtlasBold;
   GlyphAtlas m_glyphAtlas;
   Sprite m_sprite;
   TextRenderer m_textRenderer;
   TextObject m_titleLabel;
   TextObject m_framerateLabel;
   TextObject m_framerateValue;
   TextObject m_positionLabel;
   TextObject m_positionValue;
   TextObject m_orientationLabel;
   TextObject m_orientationValue;
   TextObject m_triangleCountLabel;
   TextObject m_triangleCountValue;
   TextObject m_aoLabel;
   TextObject m_aoValue;
   TextObject m_aaLabel;
   TextObject m_aaValue;
};

}// namespace renderer