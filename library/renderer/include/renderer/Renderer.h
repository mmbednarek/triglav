#pragma once

#include "Core.h"
#include "graphics_api/Device.h"
#include "graphics_api/PipelineBuilder.h"
#include "graphics_api/PlatformSurface.h"
#include "Renderer.h"
#include "SkyBox.h"

#include <optional>
#include <string_view>

namespace renderer {

struct Object3d
{
   graphics_api::DescriptorGroup descGroup;
   std::vector<graphics_api::Buffer> uniformBuffers;
   std::vector<graphics_api::MappedMemory> uniformBufferMappings;
};

struct RendererObjects
{
   uint32_t width{};
   uint32_t height{};
   graphics_api::DeviceUPtr device;
   graphics_api::RenderPass renderPass;
   graphics_api::Shader vertexShader;
   graphics_api::Shader fragmentShader;
   graphics_api::Pipeline pipeline;
   graphics_api::Semaphore framebufferReadySemaphore;
   graphics_api::Semaphore renderFinishedSemaphore;
   graphics_api::Fence inFlightFence;
   graphics_api::CommandList commandList;
   graphics_api::Texture texture1;
   graphics_api::Texture texture2;
   Object3d object1;
   Object3d object2;
};

class Renderer
{
 public:
   explicit Renderer(RendererObjects &&objects);
   void on_render();
   void on_resize(uint32_t width, uint32_t height);
   void on_close() const;
   void on_mouse_relative_move(float dx, float dy);
   [[nodiscard]] CompiledMesh compile_mesh(const Mesh &mesh) const;
   void on_mouse_wheel_turn(float x);
   [[nodiscard]] graphics_api::Texture load_texture(std::string_view path) const;
   [[nodiscard]] graphics_api::Shader load_shader(graphics_api::ShaderStage stage,
                                                  std::string_view path) const;
   [[nodiscard]] graphics_api::PipelineBuilder create_pipeline();

   template<typename TUbo>
   [[nodiscard]] graphics_api::Buffer create_ubo_buffer() const
   {
      return checkResult(m_device->create_buffer(graphics_api::BufferPurpose::UniformBuffer, sizeof(TUbo)));
   }

 private:
   void update_uniform_data(uint32_t frame);
   void write_to_texture(std::string_view path, graphics_api::Texture &texture);

   bool m_receivedMouseInput{false};
   float m_lastMouseX{};
   float m_lastMouseY{};

   float m_yaw{0};
   float m_pitch{0};
   float m_distance{12};

   uint32_t m_width{};
   uint32_t m_height{};
   graphics_api::DeviceUPtr m_device;
   graphics_api::RenderPass m_renderPass;
   graphics_api::Shader m_vertexShader;
   graphics_api::Shader m_fragmentShader;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Semaphore m_framebufferReadySemaphore;
   graphics_api::Semaphore m_renderFinishedSemaphore;
   graphics_api::Fence m_inFlightFence;
   graphics_api::CommandList m_commandList;
   graphics_api::Texture m_texture1;
   graphics_api::Texture m_texture2;
   Object3d m_object1;
   Object3d m_object2;
   std::optional<CompiledMesh> m_sphereMesh;
   std::optional<CompiledMesh> m_cilinderMesh;
   SkyBox m_skyBox;
};

Renderer init_renderer(const graphics_api::Surface &surface, uint32_t width, uint32_t height);

}// namespace renderer