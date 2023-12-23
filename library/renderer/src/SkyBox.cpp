#include "SkyBox.h"

#include "geometry/DebugMesh.h"
#include "Renderer.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <graphics_api/DescriptorWriter.h>

namespace {

graphics_api::Mesh<geometry::Vertex> create_skybox_mesh(graphics_api::Device &device)
{
   auto mesh = geometry::create_box({50, 50, 50});
   mesh.reverse_orientation();
   mesh.triangulate();
   return mesh.upload_to_device(device).mesh;
}

}// namespace

namespace renderer {

SkyBox::SkyBox(Renderer &renderer) :
    m_resourceManager(renderer.resource_manager()),
    m_mesh(create_skybox_mesh(renderer.device())),
    m_pipeline(checkResult(
            renderer.create_pipeline()
                    .fragment_shader(m_resourceManager.shader("fsh:skybox"_name))
                    .vertex_shader(m_resourceManager.shader("vsh:skybox"_name))
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
   descWriter.set_sampled_texture(1, m_resourceManager.texture("tex:skybox"_name), m_sampler);
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

}// namespace renderer