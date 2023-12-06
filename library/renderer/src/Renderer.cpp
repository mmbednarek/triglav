#include "Renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "geometry/Mesh.h"
#include "graphics_api/PipelineBuilder.h"

#include "Core.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

std::vector<uint8_t> read_whole_file(const std::string_view name)
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

renderer::Object3d create_object_3d(graphics_api::Device &device, graphics_api::Pipeline &pipeline,
                                    graphics_api::Texture &texture)
{
   auto descriptors = renderer::checkResult(pipeline.allocate_descriptors(device.framebuffer_count()));

   std::vector<graphics_api::Buffer> uniformBuffers;
   std::vector<graphics_api::MappedMemory> uniformBufferMappings;

   for (int i{}; i < device.framebuffer_count(); ++i) {
      auto buffer = renderer::checkResult(device.create_buffer(graphics_api::BufferPurpose::UniformBuffer,
                                                               sizeof(renderer::UniformBufferObject)));
      auto &movedBuffer = uniformBuffers.emplace_back(std::move(buffer));

      auto mappedMemory = renderer::checkResult(movedBuffer.map_memory());
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

constexpr auto g_colorFormat = GAPI_COLOR_FORMAT(BGRA, sRGB);
constexpr auto g_depthFormat = GAPI_COLOR_FORMAT(D, Float32);
constexpr auto g_sampleCount = graphics_api::SampleCount::Bits2;

Renderer::Renderer(RendererObjects &&objects) :
    m_width(objects.width),
    m_height(objects.height),
    m_device(std::move(objects.device)),
    m_renderPass(std::move(objects.renderPass)),
    m_vertexShader(std::move(objects.vertexShader)),
    m_fragmentShader(std::move(objects.fragmentShader)),
    m_pipeline(std::move(objects.pipeline)),
    m_framebufferReadySemaphore(std::move(objects.framebufferReadySemaphore)),
    m_renderFinishedSemaphore(std::move(objects.renderFinishedSemaphore)),
    m_inFlightFence(std::move(objects.inFlightFence)),
    m_commandList(std::move(objects.commandList)),
    m_texture1(std::move(objects.texture1)),
    m_texture2(std::move(objects.texture2)),
    m_house(std::move(objects.house)),
    m_skyBox(*this)
{
   this->write_to_texture("texture/earth.png", m_texture1);
   this->write_to_texture("texture/house.png", m_texture2);

   const auto objMesh = geometry::Mesh::from_file("model/house.obj");
   objMesh.triangulate();

   m_cilinderMesh = objMesh.upload_to_device(*m_device);
}

void Renderer::on_render()
{
   const auto framebufferIndex = m_device->get_available_framebuffer(m_framebufferReadySemaphore);
   this->update_uniform_data(framebufferIndex);
   m_inFlightFence.await();

   checkStatus(m_device->begin_graphic_commands(m_renderPass, m_commandList, framebufferIndex,
                                                graphics_api::ColorPalette::Black));

   m_skyBox.on_render(m_commandList, m_yaw, m_pitch, static_cast<float>(m_width),
                      static_cast<float>(m_height));

   m_commandList.bind_pipeline(m_pipeline);

   m_commandList.bind_descriptor_group(m_house.descGroup, framebufferIndex);
   m_commandList.draw_mesh(*m_cilinderMesh);

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

   m_pitch += dy * 0.01f;
   m_pitch = std::clamp(m_pitch, -static_cast<float>(M_PI) / 2.0f + 0.01f,
                        static_cast<float>(M_PI) / 2.0f - 0.01f);
}

void Renderer::on_key_pressed(const uint32_t key)
{
   if (key == 17) {
      m_isMovingForward = true;
   } else if (key == 31) {
      m_isMovingBackwards = true;
   } else if (key == 30) {
      m_isMovingLeft = true;
   } else if (key == 32) {
      m_isMovingRight = true;
   } else if (key == 18) {
      m_isMovingDown = true;
   } else if (key == 16) {
      m_isMovingUp = true;
   }
}

void Renderer::on_key_released(const uint32_t key)
{
   if (key == 17) {
      m_isMovingForward = false;
   } else if (key == 31) {
      m_isMovingBackwards = false;
   } else if (key == 30) {
      m_isMovingLeft = false;
   } else if (key == 32) {
      m_isMovingRight = false;
   } else if (key == 18) {
      m_isMovingDown = false;
   } else if (key == 16) {
      m_isMovingUp = false;
   }
}

void Renderer::on_mouse_wheel_turn(const float x)
{
   m_distance += x;
   m_distance = std::clamp(m_distance, 1.0f, 100.0f);
}

graphics_api::Texture Renderer::load_texture(const std::string_view path) const
{
   int texWidth, texHeight, texChannels;
   stbi_uc *pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   if (pixels == nullptr) {
      throw std::runtime_error("failed to load texture");
   }

   auto transferBuffer = checkResult(m_device->create_buffer(graphics_api::BufferPurpose::TransferBuffer,
                                                             texWidth * texHeight * sizeof(uint32_t)));
   {
      auto mapped_memory = checkResult(transferBuffer.map_memory());
      mapped_memory.write(pixels, texWidth * texHeight * sizeof(uint32_t));
   }

   auto texture = checkResult(
           m_device->create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB),
                                    {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)}));

   auto oneTimeCommands = checkResult(m_device->create_command_list());

   checkStatus(oneTimeCommands.begin_one_time());
   oneTimeCommands.copy_buffer_to_texture(transferBuffer, texture);
   checkStatus(oneTimeCommands.finish());

   checkStatus(m_device->submit_command_list_one_time(oneTimeCommands));

   return texture;
}

graphics_api::Shader Renderer::load_shader(const graphics_api::ShaderStage stage,
                                           const std::string_view path) const
{
   const auto shaderData = read_whole_file(path.data());
   if (shaderData.empty()) {
      throw std::runtime_error("failed to open vertex shader file");
   }

   return checkResult(m_device->create_shader(stage, "main", shaderData));
}

graphics_api::PipelineBuilder Renderer::create_pipeline()
{
   return {*m_device, m_renderPass};
}

void Renderer::on_resize(const uint32_t width, const uint32_t height)
{
   std::array attachments{
           graphics_api::AttachmentType::ColorAttachment,
           graphics_api::AttachmentType::DepthAttachment,
           graphics_api::AttachmentType::ResolveAttachment,
   };

   const graphics_api::Resolution resolution{width, height};

   m_device->await_all();

   m_renderPass = checkResult(m_device->create_render_pass(
           attachments, g_colorFormat, GAPI_COLOR_FORMAT(D, Float32), g_sampleCount, resolution));

   checkStatus(m_device->init_swapchain(m_renderPass, graphics_api::ColorSpace::sRGB));
   m_width  = width;
   m_height = height;
}

graphics_api::Device &Renderer::device() const
{
   return *m_device;
}

void Renderer::update_uniform_data(const uint32_t frame)
{
   const auto yVector = glm::vec4{0.0f, 1.0f, 0.0f, 1.0f};
   const auto xVector = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
   const auto zVector = glm::vec3{0.0f, 0.0f, 1.0f};
   auto yawMatrix     = glm::rotate(glm::mat4(1), m_yaw, glm::vec3{0.0f, 0.0f, 1.0f});
   auto pitchMatrix   = glm::rotate(glm::mat4(1), m_pitch, glm::vec3{1.0f, 0.0f, 0.0f});
   auto forwardVector = glm::vec3(yawMatrix * pitchMatrix * yVector);
   auto rightVector   = glm::vec3(yawMatrix * pitchMatrix * xVector);

   if (m_isMovingForward) {
      m_position += 0.005f * forwardVector;
   } else if (m_isMovingBackwards) {
      m_position -= 0.005f * forwardVector;
   } else if (m_isMovingLeft) {
      m_position -= 0.005f * rightVector;
   } else if (m_isMovingRight) {
      m_position += 0.005f * rightVector;
   } else if (m_isMovingUp) {
      m_position += 0.005f * zVector;
   } else if (m_isMovingDown) {
      m_position -= 0.005f * zVector;
   }

   auto view       = glm::lookAt(m_position, m_position + forwardVector, zVector);
   auto projection = glm::perspective(
           glm::radians(45.0f), static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 100.0f);

   UniformBufferObject houseUbo{};
   houseUbo.model = glm::rotate(glm::mat4(1.0f), glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   houseUbo.view  = view;
   houseUbo.proj  = projection;
   m_house.uniformBufferMappings[frame].write(&houseUbo, sizeof(UniformBufferObject));
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

   graphics_api::Resolution resolution{
           .width  = width,
           .height = height,
   };

   const auto [minResolution, maxResolution] = device->get_surface_resolution_limits();
   resolution.width  = std::clamp(resolution.width, minResolution.width, maxResolution.width);
   resolution.height = std::clamp(resolution.height, minResolution.height, maxResolution.height);

   std::array attachments{
           graphics_api::AttachmentType::ColorAttachment,
           graphics_api::AttachmentType::DepthAttachment,
           graphics_api::AttachmentType::ResolveAttachment,
   };

   auto renderPass = checkResult(
           device->create_render_pass(attachments, g_colorFormat, g_depthFormat, g_sampleCount, resolution));

   checkStatus(device->init_swapchain(renderPass, graphics_api::ColorSpace::sRGB));

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

   auto pipeline = checkResult(
           graphics_api::PipelineBuilder(*device, renderPass)
                   .fragment_shader(fragmentShader)
                   .vertex_shader(vertexShader)
                   // Vertex description
                   .begin_vertex_layout<geometry::Vertex>()
                   .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                   .vertex_attribute(GAPI_COLOR_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv))
                   .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal))
                   .end_vertex_layout()
                   // Descriptor layout
                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                       graphics_api::ShaderStage::Vertex)
                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                       graphics_api::ShaderStage::Fragment)
                   .descriptor_budget(10)
                   .enable_depth_test(true)
                   .build());

   auto texture1 = checkResult(device->create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB), {3600, 1673}));
   auto texture2 = checkResult(device->create_texture(GAPI_COLOR_FORMAT(RGBA, sRGB), {1024, 1024}));
   auto framebufferReadySemaphore = checkResult(device->create_semaphore());
   auto renderFinishedSemaphore   = checkResult(device->create_semaphore());
   auto inFlightFence             = checkResult(device->create_fence());
   auto commandList               = checkResult(device->create_command_list());
   auto house                     = create_object_3d(*device, pipeline, texture2);

   return Renderer(RendererObjects{
           .width                     = width,
           .height                    = height,
           .device                    = std::move(device),
           .renderPass                = std::move(renderPass),
           .vertexShader              = std::move(vertexShader),
           .fragmentShader            = std::move(fragmentShader),
           .pipeline                  = std::move(pipeline),
           .framebufferReadySemaphore = std::move(framebufferReadySemaphore),
           .renderFinishedSemaphore   = std::move(renderFinishedSemaphore),
           .inFlightFence             = std::move(inFlightFence),
           .commandList               = std::move(commandList),
           .texture1                  = std::move(texture1),
           .texture2                  = std::move(texture2),
           .house                     = std::move(house),
   });
}
}// namespace renderer
