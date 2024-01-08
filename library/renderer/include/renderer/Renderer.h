#pragma once

#include "font/FontManager.h"
#include "graphics_api/Device.h"
#include "graphics_api/PipelineBuilder.h"
#include "graphics_api/PlatformSurface.h"

#include "Context2D.h"
#include "Context3D.h"
#include "Core.h"
#include "DebugLinesRenderer.h"
#include "GlyphAtlas.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "ShadowMap.h"
#include "SkyBox.h"
#include "TextRenderer.h"

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

   Renderer(const graphics_api::Surface &surface, uint32_t width, uint32_t height);
   void on_render();
   void on_resize(uint32_t width, uint32_t height);
   void on_close() const;
   void on_mouse_relative_move(float dx, float dy);
   void on_key_pressed(uint32_t key);
   void on_key_released(uint32_t key);
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
   glm::vec3 m_position{};
   Moving m_moveDirection{Moving::None};

   graphics_api::DeviceUPtr m_device;

   font::FontManger m_fontManger;
   std::unique_ptr<ResourceManager> m_resourceManager;

   graphics_api::Resolution m_resolution;
   graphics_api::Swapchain m_swapchain;
   graphics_api::RenderPass m_renderPass;
   std::vector<graphics_api::Framebuffer> m_framebuffers;
   graphics_api::Semaphore m_framebufferReadySemaphore;
   graphics_api::Semaphore m_renderFinishedSemaphore;
   graphics_api::Fence m_inFlightFence;
   graphics_api::CommandList m_commandList;
   Context3D m_context3D;
   Context2D m_context2D;
   ShadowMap m_shadowMap;
   DebugLinesRenderer m_debugLinesRenderer;
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
   TextObject m_triangleCountLabel;
   TextObject m_triangleCountValue;
};

}// namespace renderer