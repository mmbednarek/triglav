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

template<typename TObject>
TObject checkResult(std::expected<TObject, graphics_api::Status> &&object)
{
   if (not object.has_value()) {
      throw std::runtime_error("failed to init graphics_api object");
   }
   return std::move(object.value());
}

void checkStatus(graphics_api::Status status)
{
   if (status != graphics_api::Status::Success) {
      throw std::runtime_error("failed to init graphics_api object");
   }
}

}// namespace

namespace renderer {

Renderer::Renderer(RendererObjects &&objects) :
    m_width(objects.width),
    m_height(objects.height),
    m_device(std::move(objects.device)),
    m_vertexShader(std::move(objects.vertexShader)),
    m_fragmentShader(std::move(objects.fragmentShader)),
    m_pipeline(std::move(objects.pipeline)),
    m_vertexBuffer(std::move(objects.vertexBuffer)),
    m_indexBuffer(std::move(objects.indexBuffer)),
    m_framebufferReadySemaphore(std::move(objects.framebufferReadySemaphore)),
    m_renderFinishedSemaphore(std::move(objects.renderFinishedSemaphore)),
    m_inFlightFence(std::move(objects.inFlightFence)),
    m_commandList(std::move(objects.commandList)),
    m_texture(std::move(objects.texture)),
    m_uniformBuffers(std::move(objects.uniformBuffers)),
    m_uniformBufferMappings(std::move(objects.uniformBufferMappings))
{
   this->update_vertex_data();
   this->write_to_texture();
}

void Renderer::on_render()
{
   const auto framebufferIndex = m_device->get_available_framebuffer(m_framebufferReadySemaphore);
   this->update_uniform_data(framebufferIndex);
   m_inFlightFence.await();

   checkStatus(m_device->begin_graphic_commands(m_commandList, framebufferIndex,
                                                graphics_api::ColorPalette::Black));

   m_commandList.bind_pipeline(m_pipeline, framebufferIndex);
   m_commandList.bind_vertex_buffer(m_vertexBuffer, 0);
   m_commandList.bind_index_buffer(m_indexBuffer);
   m_commandList.draw_indexed_primitives(6, 0, 0);

   checkStatus(m_commandList.finish());
   checkStatus(m_device->submit_command_list(m_commandList, m_framebufferReadySemaphore,
                                             m_renderFinishedSemaphore, m_inFlightFence));
   checkStatus(m_device->present(m_renderFinishedSemaphore, framebufferIndex));
}

void Renderer::on_close() const
{
   m_inFlightFence.await();
}

void Renderer::on_resize(uint32_t width, uint32_t height) const
{
   checkStatus(m_device->init_swapchain(graphics_api::Resolution{width, height}));
}

void Renderer::update_vertex_data()
{
   std::array<Vertex, 4> vertices{
           Vertex
           {{-0.5f, -0.5f, 0.0f}, {0, 0}},
           {{-0.5f, 0.5f, 0.0f}, {0, 1}},
           {{0.5f, 0.5f, 0.0f}, {1, 1}},
           {{0.5f, -0.5f, 0.0f}, {1, 0}},
   };

   auto transferBuffer = checkResult(
           m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer, sizeof(vertices)));
   {
      auto mappedMemory = checkResult(transferBuffer.map_memory());
      mappedMemory.write(vertices.data(), sizeof(vertices));
   }

   auto oneTimeCommands = checkResult(m_device->create_command_list());

   checkStatus(oneTimeCommands.begin_one_time());
   oneTimeCommands.copy_buffer(transferBuffer, m_vertexBuffer);
   checkStatus(oneTimeCommands.finish());

   checkStatus(m_device->submit_command_list_one_time(oneTimeCommands));

   const std::array<uint32_t, 6> indices{
           0, 2, 1,
           0, 3, 2,
   };
   auto indexTransferBuffer = checkResult(m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer, sizeof(indices)));
   {
      auto mappedMemory = checkResult(indexTransferBuffer.map_memory());
      mappedMemory.write(indices.data(), sizeof(indices));
   }

   checkStatus(oneTimeCommands.reset());
   checkStatus(oneTimeCommands.begin_one_time());
   oneTimeCommands.copy_buffer(indexTransferBuffer, m_indexBuffer);
   checkStatus(oneTimeCommands.finish());

   checkStatus(m_device->submit_command_list_one_time(oneTimeCommands));
}

void Renderer::update_uniform_data(uint32_t frame)
{
   static auto startTime = std::chrono::high_resolution_clock::now();
   auto currentTime      = std::chrono::high_resolution_clock::now();
   float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

   UniformBufferObject object{};
   object.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   object.view =
           glm::lookAt(glm::vec3(2.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   object.proj = glm::perspective(glm::radians(45.0f),
                                  static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 10.0f);

   m_uniformBufferMappings[frame].write(&object, sizeof(UniformBufferObject));
}

void Renderer::write_to_texture()
{
   int texWidth, texHeight, texChannels;
   stbi_uc *pixels = stbi_load("texture/cat.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   if (pixels == nullptr)
      return;

   auto transferBuffer = checkResult(m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer,
                                                             texWidth * texHeight * sizeof(uint32_t)));
   {
      auto mapped_memory = checkResult(transferBuffer.map_memory());
      mapped_memory.write(pixels, texWidth * texHeight * sizeof(uint32_t));
   }

   auto oneTimeCommands = checkResult(m_device->create_command_list());

   checkStatus(oneTimeCommands.begin_one_time());
   oneTimeCommands.copy_buffer_to_texture(transferBuffer, m_texture);
   checkStatus(oneTimeCommands.finish());

   checkStatus(m_device->submit_command_list_one_time(oneTimeCommands));
}

Renderer init_renderer(const graphics_api::Surface &surface, uint32_t width, uint32_t height)
{
   auto device = checkResult(graphics_api::initialize_device(surface));

   checkStatus(device->init_color_format(GAPI_COLOR_FORMAT(BGRA, sRGB), graphics_api::ColorSpace::sRGB));

   graphics_api::Resolution resolution{
           .width  = width,
           .height = height,
   };

   const auto [minResolution, maxResolution] = device->get_surface_resolution_limits();
   resolution.width  = std::clamp(resolution.width, minResolution.width, maxResolution.width);
   resolution.height = std::clamp(resolution.height, minResolution.height, maxResolution.height);

   checkStatus(device->init_swapchain(resolution));

   const auto vertexShaderData = read_whole_file("shader/example_vertex.spv");
   if (vertexShaderData.empty()) {
      throw std::runtime_error("failed to open vertex shader file");
   }

   auto vertexShader =
           checkResult(device->create_shader(graphics_api::ShaderStage::Vertex, "main", vertexShaderData));

   const auto fragmentShaderData = read_whole_file("shader/example_fragment.spv");
   if (fragmentShaderData.empty()) {
      throw std::runtime_error("failed to open fragment shader file");
   }

   auto fragmentShader = checkResult(
           device->create_shader(graphics_api::ShaderStage::Fragment, "main", fragmentShaderData));

   std::array shaders{
           &vertexShader,
           &fragmentShader,
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
                                              .offset   = offsetof(Vertex,       uv),
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

   auto pipeline = checkResult(device->create_pipeline(shaders, vertex_layout, descriptor_bindings));

   std::vector<graphics_api::Buffer> uniformBuffers;
   std::vector<graphics_api::MappedMemory> uniformBufferMappings;

   for (int i{}; i < device->framebuffer_count(); ++i) {
      auto buffer = checkResult(
              device->create_buffer(graphics_api::BufferPurpose::UniformBuffer, sizeof(UniformBufferObject)));
      auto &movedBuffer = uniformBuffers.emplace_back(std::move(buffer));

      auto mappedMemory = checkResult(movedBuffer.map_memory());
      uniformBufferMappings.emplace_back(std::move(mappedMemory));
   }

   auto texture = checkResult(device->create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB), {700, 700}));

   for (int i = 0; i < device->framebuffer_count(); ++i) {
      std::array<graphics_api::DescriptorWrite, 2> writes{
              graphics_api::DescriptorWrite{
                                            .type    = graphics_api::DescriptorType::UniformBuffer,
                                            .binding = 0,
                                            .data    = &uniformBuffers[i],
                                            },
              {
                                            .type    = graphics_api::DescriptorType::ImageSampler,
                                            .binding = 1,
                                            .data    = &texture,
                                            }
      };
      pipeline.update_descriptors(i, writes);
   }

   auto oneTimeCommandList   = checkResult(device->create_command_list());
   auto vertexBuffer =
           checkResult(device->create_buffer(graphics_api::BufferPurpose::VertexBuffer, 4 * sizeof(Vertex)));
   auto indexBuffer = checkResult(device->create_buffer(graphics_api::BufferPurpose::IndexBuffer, 6 * sizeof(uint32_t)));
   auto framebufferReadySemaphore = checkResult(device->create_semaphore());
   auto renderFinishedSemaphore   = checkResult(device->create_semaphore());
   auto inFlightFence             = checkResult(device->create_fence());
   auto commandList               = checkResult(device->create_command_list());

   return Renderer(RendererObjects{
           .width                     = width,
           .height                    = height,
           .device                    = std::move(device),
           .vertexShader              = std::move(vertexShader),
           .fragmentShader            = std::move(fragmentShader),
           .pipeline                  = std::move(pipeline),
           .vertexBuffer              = std::move(vertexBuffer),
           .indexBuffer               = std::move(indexBuffer),
           .framebufferReadySemaphore = std::move(framebufferReadySemaphore),
           .renderFinishedSemaphore   = std::move(renderFinishedSemaphore),
           .inFlightFence             = std::move(inFlightFence),
           .commandList               = std::move(commandList),
           .texture                   = std::move(texture),
           .uniformBuffers            = std::move(uniformBuffers),
           .uniformBufferMappings     = std::move(uniformBufferMappings),
   });
}
}// namespace renderer
