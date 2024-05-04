#include "SkyBox.h"

#include "Renderer.h"
#include "triglav/geometry/DebugMesh.h"
#include "triglav/graphics_api/DescriptorWriter.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::render_core::UniformBufferObject;
using namespace triglav::name_literals;

namespace {

triglav::graphics_api::Mesh<triglav::geometry::Vertex>
create_skybox_mesh(triglav::graphics_api::Device &device)
{
   auto mesh = triglav::geometry::create_box({50, 50, 50});
   mesh.reverse_orientation();
   mesh.triangulate();
   return mesh.upload_to_device(device).mesh;
}

}// namespace

namespace triglav::renderer {

SkyBox::SkyBox(graphics_api::Device &device, resource::ResourceManager &resourceManager,
               graphics_api::RenderTarget &renderTarget) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_mesh(create_skybox_mesh(device)),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(device, renderTarget)
                    .fragment_shader(
                            m_resourceManager.get<ResourceType::FragmentShader>("skybox.fshader"_name))
                    .vertex_shader(m_resourceManager.get<ResourceType::VertexShader>("skybox.vshader"_name))
                    // Vertex description
                    .begin_vertex_layout<geometry::Vertex>()
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                    .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv))
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal))
                    .end_vertex_layout()
                    // Descriptor layout
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::PipelineStage::VertexShader)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::PipelineStage::FragmentShader)
                    .enable_depth_test(false)
                    .use_push_descriptors(true)
                    .build())),
    m_sampler(m_resourceManager.get<ResourceType::Sampler>("linear_repeat_mlod0.sampler"_name)),
    m_texture(m_resourceManager.get<ResourceType::Texture>("skybox.tex"_name))
{
}

void SkyBox::on_render(graphics_api::CommandList &commandList, UniformBuffer& ubo, float yaw, float pitch, float width,
                       float height) const
{
   static constexpr auto yVector = glm::vec4{0.0f, 1.0f, 0, 1.0f};
   const auto yawMatrix          = glm::rotate(glm::mat4(1), yaw, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto pitchMatrix        = glm::rotate(glm::mat4(1), pitch, glm::vec3{1.0f, 0.0f, 0.0f});
   const auto forwardVector      = glm::vec3(yawMatrix * pitchMatrix * yVector);

   const auto view       = glm::lookAt(glm::vec3{0, 0, 0}, forwardVector, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto projection = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);

   ubo->view = view;
   ubo->proj = projection;

   commandList.bind_pipeline(m_pipeline);

   graphics_api::DescriptorWriter descWriter(m_device);
   descWriter.set_uniform_buffer(0, ubo);
   descWriter.set_sampled_texture(1, m_texture, m_sampler);
   commandList.push_descriptors(0, descWriter);

   commandList.draw_mesh(m_mesh);
}

}// namespace triglav::renderer