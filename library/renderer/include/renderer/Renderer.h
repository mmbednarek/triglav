#pragma once

#include "graphics_api/Device.h"
#include "graphics_api/PlatformSurface.h"

#include <optional>

namespace renderer {

struct Object3d {
   graphics_api::DescriptorGroup descGroup;
   std::vector<graphics_api::Buffer> uniformBuffers;
   std::vector<graphics_api::MappedMemory> uniformBufferMappings;
};

struct RendererObjects {
   uint32_t width{};
   uint32_t height{};
   graphics_api::DeviceUPtr device;
   graphics_api::Shader vertexShader;
   graphics_api::Shader fragmentShader;
   graphics_api::Pipeline pipeline;
   graphics_api::Buffer vertexBuffer;
   graphics_api::Buffer indexBuffer;
   graphics_api::Semaphore framebufferReadySemaphore;
   graphics_api::Semaphore renderFinishedSemaphore;
   graphics_api::Fence inFlightFence;
   graphics_api::CommandList commandList;
   graphics_api::Texture texture;
   Object3d object1;
   Object3d object2;
};

class Renderer
{
 public:
   explicit Renderer(RendererObjects&& objects);
   void on_render();
   void on_resize(uint32_t width, uint32_t height) const;
   void on_close() const;

 private:
   void update_vertex_data();
   void update_uniform_data(uint32_t frame);
   void write_to_texture();

   uint32_t m_width{};
   uint32_t m_height{};
   graphics_api::DeviceUPtr m_device;
   graphics_api::Shader m_vertexShader;
   graphics_api::Shader m_fragmentShader;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Buffer m_vertexBuffer;
   graphics_api::Buffer m_indexBuffer;
   graphics_api::Semaphore m_framebufferReadySemaphore;
   graphics_api::Semaphore m_renderFinishedSemaphore;
   graphics_api::Fence m_inFlightFence;
   graphics_api::CommandList m_commandList;
   graphics_api::Texture m_texture;
   Object3d m_object1;
   Object3d m_object2;
};

Renderer init_renderer(const graphics_api::Surface& surface, uint32_t width, uint32_t height);

}