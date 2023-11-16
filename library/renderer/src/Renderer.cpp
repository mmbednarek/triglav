#include "Renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../include/renderer/Core.h"
#include "../include/renderer/DebugMeshes.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

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

renderer::Object3d create_object_3d(graphics_api::Device &device, graphics_api::Pipeline &pipeline,
                                    graphics_api::Texture &texture)
{
   auto descriptors = checkResult(pipeline.allocate_descriptors());

   std::vector<graphics_api::Buffer> uniformBuffers;
   std::vector<graphics_api::MappedMemory> uniformBufferMappings;

   for (int i{}; i < device.framebuffer_count(); ++i) {
      auto buffer       = checkResult(device.create_buffer(graphics_api::BufferPurpose::UniformBuffer,
                                                           sizeof(renderer::UniformBufferObject)));
      auto &movedBuffer = uniformBuffers.emplace_back(std::move(buffer));

      auto mappedMemory = checkResult(movedBuffer.map_memory());
      uniformBufferMappings.emplace_back(std::move(mappedMemory));
   }

   for (int i = 0; i < device.framebuffer_count(); ++i) {
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
      descriptors.update(i, writes);
   }

   return renderer::Object3d{.descGroup             = std::move(descriptors),
                             .uniformBuffers        = std::move(uniformBuffers),
                             .uniformBufferMappings = std::move(uniformBufferMappings)};
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
    m_framebufferReadySemaphore(std::move(objects.framebufferReadySemaphore)),
    m_renderFinishedSemaphore(std::move(objects.renderFinishedSemaphore)),
    m_inFlightFence(std::move(objects.inFlightFence)),
    m_commandList(std::move(objects.commandList)),
    m_texture1(std::move(objects.texture1)),
    m_texture2(std::move(objects.texture2)),
    m_object1(std::move(objects.object1)),
    m_object2(std::move(objects.object2))
{
   this->write_to_texture("texture/earth.png", m_texture1);
   this->write_to_texture("texture/moon.png", m_texture2);

   m_sphereMesh   = this->compile_mesh(create_sphere(48, 24, 3.0f));
   m_cilinderMesh = this->compile_mesh(create_sphere(32, 16, 1.0f));
}

void Renderer::on_render()
{
   const auto framebufferIndex = m_device->get_available_framebuffer(m_framebufferReadySemaphore);
   this->update_uniform_data(framebufferIndex);
   m_inFlightFence.await();

   checkStatus(m_device->begin_graphic_commands(m_commandList, framebufferIndex,
                                                graphics_api::ColorPalette::Black));

   m_commandList.bind_pipeline(m_pipeline);

   m_commandList.bind_descriptor_group(m_object1.descGroup, framebufferIndex);
   m_commandList.bind_vertex_buffer(m_sphereMesh->vertex_buffer, 0);
   m_commandList.bind_index_buffer(m_sphereMesh->index_buffer);
   m_commandList.draw_indexed_primitives(m_sphereMesh->index_count, 0, 0);

   m_commandList.bind_descriptor_group(m_object2.descGroup, framebufferIndex);
   m_commandList.bind_vertex_buffer(m_cilinderMesh->vertex_buffer, 0);
   m_commandList.bind_index_buffer(m_cilinderMesh->index_buffer);
   m_commandList.draw_indexed_primitives(m_cilinderMesh->index_count, 0, 0);

   checkStatus(m_commandList.finish());
   checkStatus(m_device->submit_command_list(m_commandList, m_framebufferReadySemaphore,
                                             m_renderFinishedSemaphore, m_inFlightFence));
   checkStatus(m_device->present(m_renderFinishedSemaphore, framebufferIndex));
}

void Renderer::on_close() const
{
   m_inFlightFence.await();
}

void Renderer::on_mouse_relative_move(const float dx, const float dy)
{
   m_yaw -= dx * 0.01f;
   while (m_yaw < 0) {
      m_yaw += 2 * M_PI;
   }
   while (m_yaw >= 2 * M_PI) {
      m_yaw -= 2 * M_PI;
   }

   m_pitch -= dy * 0.01f;
   m_pitch = std::clamp(m_pitch, -static_cast<float>(M_PI) / 2.0f + 0.01f, static_cast<float>(M_PI) / 2.0f - 0.01f);
}

CompiledMesh Renderer::compile_mesh(const Mesh &mesh) const
{
   auto vertexBuffer   = checkResult(m_device->create_buffer(graphics_api::BufferPurpose::VertexBuffer,
                                                             sizeof(Vertex) * mesh.vertices.size()));
   auto transferBuffer = checkResult(m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer,
                                                             sizeof(Vertex) * mesh.vertices.size()));
   {
      auto mappedMemory = checkResult(transferBuffer.map_memory());
      mappedMemory.write(mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size());
   }

   auto oneTimeCommands = checkResult(m_device->create_command_list());

   checkStatus(oneTimeCommands.begin_one_time());
   oneTimeCommands.copy_buffer(transferBuffer, vertexBuffer);
   checkStatus(oneTimeCommands.finish());
   checkStatus(m_device->submit_command_list_one_time(oneTimeCommands));

   auto indexBuffer         = checkResult(m_device->create_buffer(graphics_api::BufferPurpose::IndexBuffer,
                                                                  sizeof(uint32_t) * mesh.indicies.size()));
   auto indexTransferBuffer = checkResult(m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer,
                                                                  sizeof(uint32_t) * mesh.indicies.size()));
   {
      auto mappedMemory = checkResult(indexTransferBuffer.map_memory());
      mappedMemory.write(mesh.indicies.data(), sizeof(uint32_t) * mesh.indicies.size());
   }

   checkStatus(oneTimeCommands.reset());
   checkStatus(oneTimeCommands.begin_one_time());
   oneTimeCommands.copy_buffer(indexTransferBuffer, indexBuffer);
   checkStatus(oneTimeCommands.finish());
   checkStatus(m_device->submit_command_list_one_time(oneTimeCommands));

   return CompiledMesh{std::move(indexBuffer), std::move(vertexBuffer), mesh.indicies.size()};
}

void Renderer::on_mouse_wheel_turn(const float x)
{
   m_distance += x;
   m_distance = std::clamp(m_distance, 1.0f, 100.0f);
}

void Renderer::on_resize(const uint32_t width, const uint32_t height)
{
   checkStatus(m_device->init_swapchain(graphics_api::Resolution{width, height}));
   m_width  = width;
   m_height = height;
}

void Renderer::update_uniform_data(uint32_t frame)
{
   static auto startTime = std::chrono::high_resolution_clock::now();
   auto currentTime      = std::chrono::high_resolution_clock::now();
   float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

   const auto eye = glm::vec4{0.0f, m_distance, 0, 1.0f};
   auto eye_yaw   = glm::rotate(glm::mat4(1), m_yaw, glm::vec3{0.0f, 0.0f, 1.0f});
   auto eye_pitch = glm::rotate(glm::mat4(1), m_pitch, glm::vec3{1.0f, 0.0f, 0.0f});
   auto eye_final = eye_yaw * eye_pitch * eye;

   auto view = glm::lookAt(glm::vec3(eye_final), glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
   auto projection = glm::perspective(
           glm::radians(45.0f), static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 100.0f);

   UniformBufferObject object1{};
   object1.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   object1.view  = view;
   object1.proj  = projection;
   m_object1.uniformBufferMappings[frame].write(&object1, sizeof(UniformBufferObject));

   UniformBufferObject object2{};
   auto rot      = glm::rotate(glm::mat4(1.0f), -time * glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   auto trans    = glm::translate(rot, glm::vec3(0.0f, 8.0f, 0.0f));
   object2.model = trans;
   object2.view  = view;
   object2.proj  = projection;
   m_object2.uniformBufferMappings[frame].write(&object2, sizeof(UniformBufferObject));
}

void Renderer::write_to_texture(std::string_view path, graphics_api::Texture &texture)
{
   int texWidth, texHeight, texChannels;
   stbi_uc *pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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
   oneTimeCommands.copy_buffer_to_texture(transferBuffer, texture);
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

   std::array<graphics_api::VertexInputAttribute, 3> vertex_attributes{
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
           graphics_api::VertexInputAttribute{
                                              .location = 2,
                                              .format   = GAPI_COLOR_FORMAT(RGB, Float32),
                                              .offset   = offsetof(Vertex,   normal),
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

   auto pipeline = checkResult(device->create_pipeline(shaders, vertex_layout, descriptor_bindings, 10));
   auto texture1 = checkResult(device->create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB), {3600, 1673}));
   auto texture2 = checkResult(device->create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB), {2048, 1024}));
   auto framebufferReadySemaphore = checkResult(device->create_semaphore());
   auto renderFinishedSemaphore   = checkResult(device->create_semaphore());
   auto inFlightFence             = checkResult(device->create_fence());
   auto commandList               = checkResult(device->create_command_list());
   auto object1                   = create_object_3d(*device, pipeline, texture1);
   auto object2                   = create_object_3d(*device, pipeline, texture2);

   return Renderer(RendererObjects{
           .width                     = width,
           .height                    = height,
           .device                    = std::move(device),
           .vertexShader              = std::move(vertexShader),
           .fragmentShader            = std::move(fragmentShader),
           .pipeline                  = std::move(pipeline),
           .framebufferReadySemaphore = std::move(framebufferReadySemaphore),
           .renderFinishedSemaphore   = std::move(renderFinishedSemaphore),
           .inFlightFence             = std::move(inFlightFence),
           .commandList               = std::move(commandList),
           .texture1                  = std::move(texture1),
           .texture2                  = std::move(texture2),
           .object1                   = std::move(object1),
           .object2                   = std::move(object2),
   });
}
}// namespace renderer
