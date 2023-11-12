#include "Renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

struct Vertex
{
   glm::vec3 position;
   glm::vec2 uv;
};

struct UniformBufferObject
{
   alignas(16) glm::mat4 model;
   alignas(16) glm::mat4 view;
   alignas(16) glm::mat4 proj;
};

std::vector<uint8_t> read_whole_file(std::string_view name)
{
   std::ifstream file(std::string{name}, std::ios::ate | std::ios::binary);
   if (not file.is_open()) {
      return {};
   }

   file.seekg(0, std::ios::end);
   const auto fileSize = file.tellg();
   file.seekg(0, std::ios::beg);

   std::vector<uint8_t> result{};
   result.resize(fileSize);

   file.read(reinterpret_cast<char *>(result.data()), fileSize);
   return result;
}

}// namespace

namespace renderer {

bool Renderer::on_init(const graphics_api::Surface &surface, uint32_t width, uint32_t height)
{
   m_width  = width;
   m_height = height;

   m_device = graphics_api::initialize_device(surface).value_or(nullptr);
   if (m_device == nullptr) {
      return false;
   }

   if (m_device->init_color_format(GAPI_COLOR_FORMAT(BGRA, sRGB), graphics_api::ColorSpace::sRGB) !=
       graphics_api::Status::Success) {
      return false;
   }

   graphics_api::Resolution resolution{
           .width  = width,
           .height = height,
   };

   const auto [minResolution, maxResolution] = m_device->get_surface_resolution_limits();
   resolution.width  = std::clamp(resolution.width, minResolution.width, maxResolution.width);
   resolution.height = std::clamp(resolution.height, minResolution.height, maxResolution.height);

   if (m_device->init_swapchain(resolution) != graphics_api::Status::Success) {
      return false;
   }

   const auto vertexShaderData = read_whole_file("shader/example_vertex.spv");
   if (vertexShaderData.empty()) {
      return false;
   }

   auto vertexShader = m_device->create_shader(graphics_api::ShaderStage::Vertex, "main", vertexShaderData);
   if (not vertexShader.has_value()) {
      return false;
   }
   m_vertexShader = std::move(*vertexShader);

   const auto fragmentShaderData = read_whole_file("shader/example_fragment.spv");
   if (fragmentShaderData.empty()) {
      return false;
   }

   auto fragmentShader =
           m_device->create_shader(graphics_api::ShaderStage::Fragment, "main", fragmentShaderData);
   if (not fragmentShader.has_value()) {
      return false;
   }
   m_fragmentShader = std::move(*fragmentShader);

   std::array shaders{
           &(*m_vertexShader),
           &(*m_fragmentShader),
   };

   std::array<graphics_api::VertexInputAttribute, 2> vertex_attributes{
           graphics_api::VertexInputAttribute{
                                              .location = 0,
                                              .format   = GAPI_COLOR_FORMAT(RGB, Float32),
                                              .offset   = offsetof(Vertex, position),
                                              },
           graphics_api::VertexInputAttribute{
                   .location = 1,
                   .format   = GAPI_COLOR_FORMAT(RG, Float32),
                   .offset   = offsetof(Vertex, uv),
           },
   };
   std::array<graphics_api::VertexInputLayout, 1> vertex_layout{
           graphics_api::VertexInputLayout{
                                           .attributes     = vertex_attributes,
                                           .structure_size = sizeof(Vertex),
                                           },
   };

   std::array<graphics_api::DescriptorBinding, 2> descriptor_bindings{
           graphics_api::DescriptorBinding{
                                           .binding         = 0,
                                           .descriptorCount = 1,
                                           .type            = graphics_api::DescriptorType::UniformBuffer,
                                           .shaderStages =
                                           static_cast<graphics_api::ShaderStageFlags>(graphics_api::ShaderStage::Vertex),
                                           },
           graphics_api::DescriptorBinding{
                                           .binding         = 1,
                                           .descriptorCount = 1,
                                           .type            = graphics_api::DescriptorType::ImageSampler,
                                           .shaderStages =
                                           static_cast<graphics_api::ShaderStageFlags>(graphics_api::ShaderStage::Fragment),
                                           },
   };

   auto pipeline = m_device->create_pipeline(shaders, vertex_layout, descriptor_bindings);
   if (not pipeline.has_value()) {
      return false;
   }
   m_pipeline = std::move(*pipeline);

   for (int i{}; i < m_device->framebuffer_count(); ++i) {
      auto buffer = m_device->create_buffer(graphics_api::BufferPurpose::UniformBuffer,
                                            sizeof(UniformBufferObject));
      if (not buffer.has_value()) {
         return false;
      }
      auto &movedBuffer = m_uniformBuffers.emplace_back(std::move(*buffer));

      auto mappedMemory = movedBuffer.map_memory();
      if (not mappedMemory.has_value()) {
         return false;
      }
      m_uniformBufferMappings.emplace_back(std::move(*mappedMemory));
   }

   auto texture = m_device->create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB), {700, 700});
   if (not texture.has_value()) {
      return false;
   }
   m_texture = std::move(*texture);

   for (int i = 0; i < m_device->framebuffer_count(); ++i) {
      std::array<graphics_api::DescriptorWrite, 2> writes{
              graphics_api::DescriptorWrite{
                                            .type    = graphics_api::DescriptorType::UniformBuffer,
                                            .binding = 0,
                                            .data    = &m_uniformBuffers[i],
                                            },
                                            {
                                            .type    = graphics_api::DescriptorType::ImageSampler,
                                            .binding = 1,
                                            .data    = &(*m_texture),
                                            }
      };
      m_pipeline->update_descriptors(i, writes);
   }

   auto oneTimeCommandList = m_device->create_command_list();
   if (not oneTimeCommandList.has_value()) {
      return false;
   }
   m_oneTimeCommandList = std::move(*oneTimeCommandList);

   auto vertexTransferBuffer =
           m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer, 6 * sizeof(Vertex));
   if (not vertexTransferBuffer.has_value())
      return false;
   m_vertexTransferBuffer = std::move(*vertexTransferBuffer);

   auto vertexBuffer = m_device->create_buffer(graphics_api::BufferPurpose::VertexBuffer, 6 * sizeof(Vertex));
   if (not vertexBuffer.has_value())
      return false;
   m_vertexBuffer = std::move(*vertexBuffer);

   this->update_vertex_data();

   auto waitSemaphore = m_device->create_semaphore();
   if (not waitSemaphore.has_value()) {
      return false;
   }
   m_framebufferReadySemaphore = std::move(*waitSemaphore);

   auto signalSemaphore = m_device->create_semaphore();
   if (not signalSemaphore.has_value()) {
      return false;
   }
   m_renderFinishedSemaphore = std::move(*signalSemaphore);

   auto fence = m_device->create_fence();
   if (not fence.has_value()) {
      return false;
   }
   m_inFlightFence = std::move(*fence);

   auto commandList = m_device->create_command_list();
   if (not commandList.has_value()) {
      return false;
   }
   m_commandList = std::move(*commandList);

   this->write_to_texture();

   this->update_vertex_data();

   return true;
}

bool Renderer::on_render()
{
   const auto framebufferIndex = m_device->get_available_framebuffer(*m_framebufferReadySemaphore);

   this->update_uniform_data(framebufferIndex);

   m_inFlightFence->await();

   if (m_device->begin_graphic_commands(*m_commandList, framebufferIndex,
                                        graphics_api::ColorPalette::Black) != graphics_api::Status::Success) {
      return false;
   }

   m_commandList->bind_pipeline(*m_pipeline, framebufferIndex);
   m_commandList->bind_vertex_buffer(*m_vertexBuffer, 0);
   m_commandList->draw_primitives(6, 0);

   if (m_commandList->finish() != graphics_api::Status::Success) {
      return false;
   }

   if (m_device->submit_command_list(*m_commandList, *m_framebufferReadySemaphore, *m_renderFinishedSemaphore,
                                     *m_inFlightFence) != graphics_api::Status::Success) {
      return false;
   }

   if (m_device->present(*m_renderFinishedSemaphore, framebufferIndex) != graphics_api::Status::Success) {
      return false;
   }

   return true;
}

void Renderer::on_close() const
{
   m_inFlightFence->await();
}

bool Renderer::on_resize(uint32_t width, uint32_t height) const
{
   const auto result = m_device->init_swapchain(graphics_api::Resolution{width, height});
   if (result != graphics_api::Status::Success)
      return false;

   //   const auto result = m_device->create_pipeline();

   return true;
}

void Renderer::update_vertex_data()
{
   std::array<Vertex, 6> vertices{
           Vertex{{-0.5f, -0.5f, 0.0f}, {0, 0}}, {{0.5f, -0.5f, 0.0f}, {1, 0}}, {{0.5f, 0.5f, 0.0f}, {1, 1}},
           Vertex{{-0.5f, -0.5f, 0.0f}, {0, 0}}, {{0.5f, 0.5f, 0.0f}, {1, 1}},  {{-0.5f, 0.5f, 0.0f}, {0, 1}},
   };

   auto mappedMemory = m_vertexTransferBuffer->map_memory();
   if (not mappedMemory.has_value())
      return;
   mappedMemory->write(vertices.data(), sizeof(vertices));

   if (m_device->begin_one_time_commands(*m_oneTimeCommandList) != graphics_api::Status::Success)
      return;

   m_oneTimeCommandList->copy_buffer(*m_vertexTransferBuffer, *m_vertexBuffer);

   if (m_oneTimeCommandList->finish() != graphics_api::Status::Success)
      return;
   if (m_device->submit_command_list_one_time(*m_oneTimeCommandList) != graphics_api::Status::Success)
      return;
}

void Renderer::update_uniform_data(uint32_t frame)
{
   static auto startTime = std::chrono::high_resolution_clock::now();
   auto currentTime      = std::chrono::high_resolution_clock::now();
   float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

   UniformBufferObject object{};
   object.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   object.view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 0.0f, 1.0f));
   object.proj  = glm::perspective(glm::radians(45.0f),
                                   static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 10.0f);

   m_uniformBufferMappings[frame].write(&object, sizeof(UniformBufferObject));
}

void Renderer::write_to_texture()
{
   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load("texture/cat.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   if (pixels == nullptr)
      return;

   auto buffer =
           m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer, texWidth * texHeight * sizeof(uint32_t));
   if (not buffer.has_value())
      return;

   {
      auto mapped_memory = buffer->map_memory();
      if (not mapped_memory.has_value())
         return;

      mapped_memory->write(pixels, texWidth * texHeight * sizeof(uint32_t));
//      auto *data = reinterpret_cast<uint32_t *>(**mapped_memory);
//      for (int y = 0; y < 512; ++y) {
//         for (int x = 0; x < 512; ++x) {
//            data[x + 512 * y] = (x + y) % 2 == 0 ? 0xFF00FFFF : 0x00FF00FF;
//         }
//      }
   }

   m_device->begin_one_time_commands(*m_oneTimeCommandList);
   m_oneTimeCommandList->copy_buffer_to_texture(*buffer, *m_texture);
   m_oneTimeCommandList->finish();

   m_device->submit_command_list_one_time(*m_oneTimeCommandList);
}

}// namespace renderer
