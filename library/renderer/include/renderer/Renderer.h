#pragma once

#include "graphics_api/Device.h"
#include "graphics_api/PlatformSurface.h"

#include <optional>

namespace renderer {

class Renderer
{
 public:
   [[nodiscard]] bool on_init(const graphics_api::Surface& surface, uint32_t width, uint32_t height);
   [[nodiscard]] bool on_render();
   [[nodiscard]] bool on_resize(uint32_t width, uint32_t height) const;
   void on_close() const;

 private:
   void update_vertex_data();
   void update_uniform_data(uint32_t frame);
   void write_to_texture();

   uint32_t m_width{};
   uint32_t m_height{};
   graphics_api::DeviceUPtr m_device;
   std::optional<graphics_api::Shader> m_vertexShader;
   std::optional<graphics_api::Shader> m_fragmentShader;
   std::optional<graphics_api::Pipeline> m_pipeline;
   std::optional<graphics_api::Buffer> m_vertexTransferBuffer;
   std::optional<graphics_api::Buffer> m_vertexBuffer;
   std::optional<graphics_api::Semaphore> m_framebufferReadySemaphore;
   std::optional<graphics_api::Semaphore> m_renderFinishedSemaphore;
   std::optional<graphics_api::Fence> m_inFlightFence;
   std::optional<graphics_api::CommandList> m_commandList;
   std::optional<graphics_api::CommandList> m_oneTimeCommandList;
   std::optional<graphics_api::Texture> m_texture;
   std::vector<graphics_api::Buffer> m_uniformBuffers;
   std::vector<graphics_api::MappedMemory> m_uniformBufferMappings;
};

}