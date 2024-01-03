#include "Context3D.h"

#include "graphics_api/Device.h"
#include "graphics_api/PipelineBuilder.h"

#include "Core.h"
#include "Name.hpp"
#include "ResourceManager.h"
#include "ShadowMap.h"

#include <graphics_api/CommandList.h>
#include <graphics_api/DescriptorWriter.h>

namespace renderer {

constexpr auto g_shadowMapResolution = graphics_api::Resolution{4096, 4096};

Context3D::Context3D(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                     ResourceManager &resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(m_device, renderPass)
                    .fragment_shader(resourceManager.shader("fsh:model"_name))
                    .vertex_shader(resourceManager.shader("vsh:model"_name))
                    // Vertex description
                    .begin_vertex_layout<geometry::Vertex>()
                    .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                    .vertex_attribute(GAPI_COLOR_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv))
                    .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal))
                    .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, tangent))
                    .vertex_attribute(GAPI_COLOR_FORMAT(RGB, Float32), offsetof(geometry::Vertex, bitangent))
                    .end_vertex_layout()
                    // Descriptor layout
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::ShaderStage::Vertex)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::ShaderStage::Fragment)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::ShaderStage::Fragment)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::ShaderStage::Fragment)
                    .push_constant(graphics_api::ShaderStage::Fragment, sizeof(PushConstant))
                    .enable_depth_test(true)
                    .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(100, 100, 100))),
    m_sampler(checkResult(device.create_sampler(true)))
{
}

InstancedModel Context3D::instance_model(const Name modelName, ShadowMap &shadowMap)
{
   const auto &model = m_resourceManager.model(modelName);
   auto descriptors  = checkResult(m_descriptorPool.allocate_array(model.range.size()));
   size_t index{};

   graphics_api::UniformBuffer<UniformBufferObject> ubo(m_device);

   for (const auto range : model.range) {
      const auto &material      = m_resourceManager.material(range.materialName);
      const auto &texture       = m_resourceManager.texture(material.texture);
      const auto &normalTexture = m_resourceManager.texture(material.normal_texture);

      graphics_api::DescriptorWriter descWriter(m_device, descriptors[index]);
      descWriter.set_uniform_buffer(0, ubo);
      descWriter.set_sampled_texture(1, texture, m_sampler);
      descWriter.set_sampled_texture(2, normalTexture, m_sampler);
      descWriter.set_sampled_texture(3, shadowMap.depth_texture(), m_sampler);

      ++index;
   }

   return InstancedModel{modelName, std::move(ubo), std::move(descriptors),
                         shadowMap.create_model_properties()};
}

void Context3D::set_active_command_list(graphics_api::CommandList *commandList)
{
   m_commandList = commandList;
}

void Context3D::begin_render()
{
   m_commandList->bind_pipeline(m_pipeline);
   m_commandList->push_constant(graphics_api::ShaderStage::Fragment, m_pushConstant);
}

void Context3D::draw_model(const InstancedModel &instancedModel) const
{
   assert(m_commandList != nullptr);

   const auto &model = m_resourceManager.model(instancedModel.modelName);
   const auto &mesh  = m_resourceManager.mesh(model.meshName);

   m_commandList->bind_vertex_array(mesh.vertices);
   m_commandList->bind_index_array(mesh.indices);

   size_t index{};
   for (const auto &range : model.range) {
      const auto descriptorSet = instancedModel.descriptors[index];

      m_commandList->bind_descriptor_set(descriptorSet);
      m_commandList->draw_indexed_primitives(static_cast<int>(range.size), static_cast<int>(range.offset), 0);
      ++index;
   }
}

void Context3D::set_light_position(const glm::vec3 pos)
{
   m_pushConstant.lightPosition = pos;
}

graphics_api::CommandList &Context3D::command_list() const
{
   return *m_commandList;
}

}// namespace renderer