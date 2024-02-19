#include "SkyBox.h"

#include "triglav/geometry/DebugMesh.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "Renderer.h"

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

SkyBox::SkyBox(Renderer &renderer) :
    m_resourceManager(renderer.resource_manager()),
    m_mesh(create_skybox_mesh(renderer.device())),
    m_pipeline(checkResult(
            renderer.create_pipeline()
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
                                        graphics_api::ShaderStage::Vertex)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::ShaderStage::Fragment)
                    .enable_depth_test(false)
                    .build()

                    )),
    m_descPool(checkResult(m_pipeline.create_descriptor_pool(3, 3, 3))),
    m_descArray(checkResult(m_descPool.allocate_array(1))),
    m_descriptorSet(m_descArray[0]),
    m_uniformBuffer(renderer.create_ubo_buffer<SkyBoxUBO>()),
    m_uniformBufferMapping(checkResult(m_uniformBuffer.map_memory())),
    m_sampler(checkResult(renderer.device().create_sampler(false)))
{
   graphics_api::DescriptorWriter descWriter(renderer.device(), m_descriptorSet);
   descWriter.set_raw_uniform_buffer(0, m_uniformBuffer);
   descWriter.set_sampled_texture(1, m_resourceManager.get<ResourceType::Texture>("skybox.tex"_name),
                                  m_sampler);
}

void SkyBox::on_render(graphics_api::CommandList &commandList, float yaw, float pitch, float width,
                       float height) const
{
   static constexpr auto yVector = glm::vec4{0.0f, 1.0f, 0, 1.0f};
   const auto yawMatrix          = glm::rotate(glm::mat4(1), yaw, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto pitchMatrix        = glm::rotate(glm::mat4(1), pitch, glm::vec3{1.0f, 0.0f, 0.0f});
   const auto forwardVector      = glm::vec3(yawMatrix * pitchMatrix * yVector);

   const auto view       = glm::lookAt(glm::vec3{0, 0, 0}, forwardVector, glm::vec3{0.0f, 0.0f, 1.0f});
   const auto projection = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);

   SkyBoxUBO object1{};
   object1.view = view;
   object1.proj = projection;
   m_uniformBufferMapping.write(&object1, sizeof(UniformBufferObject));

   commandList.bind_pipeline(m_pipeline);
   commandList.bind_descriptor_set(m_descriptorSet);
   commandList.draw_mesh(m_mesh);
}

}// namespace triglav::renderer