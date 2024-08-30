#include "SkyBox.h"

#include "Renderer.h"
#include "triglav/geometry/DebugMesh.h"
#include "triglav/graphics_api/DescriptorWriter.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::render_core::UniformBufferObject;
using namespace triglav::name_literals;

namespace {

triglav::graphics_api::Mesh<triglav::geometry::Vertex> create_skybox_mesh(triglav::graphics_api::Device& device)
{
   auto mesh = triglav::geometry::create_box({50, 50, 50});
   mesh.reverse_orientation();
   mesh.triangulate();
   return mesh.upload_to_device(device).mesh;
}

}// namespace

namespace triglav::renderer {

SkyBox::SkyBox(graphics_api::Device& device, resource::ResourceManager& resourceManager, graphics_api::RenderTarget& renderTarget) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_mesh(create_skybox_mesh(device)),
    m_pipeline(checkResult(graphics_api::GraphicsPipelineBuilder(device, renderTarget)
                              .fragment_shader(m_resourceManager.get("skybox.fshader"_rc))
                              .vertex_shader(m_resourceManager.get("skybox.vshader"_rc))
                              // Vertex description
                              .begin_vertex_layout<geometry::Vertex>()
                              .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                              .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv))
                              .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal))
                              .end_vertex_layout()
                              // Descriptor layout
                              .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .enable_depth_test(false)
                              .use_push_descriptors(true)
                              .build())),
    m_texture(m_resourceManager.get("skybox.tex"_rc))
{
}

void SkyBox::on_render(graphics_api::CommandList& commandList, UniformBuffer& ubo, float yaw, float pitch, float width, float height) const
{
   static constexpr auto yVector = glm::vec4{0.0f, 1.0f, 0, 1.0f};
   const auto yawMatrix = glm::rotate(glm::mat4(1), yaw, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto pitchMatrix = glm::rotate(glm::mat4(1), pitch, glm::vec3{1.0f, 0.0f, 0.0f});
   const auto forwardVector = glm::vec3(yawMatrix * pitchMatrix * yVector);

   const auto view = glm::lookAt(glm::vec3{0, 0, 0}, forwardVector, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto projection = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);

   ubo->view = view;
   ubo->proj = projection;

   commandList.bind_pipeline(m_pipeline);

   commandList.bind_uniform_buffer(0, ubo);
   commandList.bind_texture(1, m_texture);

   commandList.draw_mesh(m_mesh);
}

}// namespace triglav::renderer