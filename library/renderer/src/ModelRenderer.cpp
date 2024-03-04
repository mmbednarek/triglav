#include "ModelRenderer.h"

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/Model.hpp"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"
#include "triglav/ResourceType.hpp"

#include "ShadowMap.h"

using triglav::Name;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::render_core::InstancedModel;
using triglav::render_core::MaterialProps;
using triglav::render_core::UniformBufferObject;
using triglav::resource::ResourceManager;

using namespace triglav::name_literals;

namespace triglav::renderer {

constexpr auto g_shadowMapResolution = graphics_api::Resolution{4096, 4096};

ModelRenderer::ModelRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                             ResourceManager &resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(m_device, renderPass)
                    .fragment_shader(resourceManager.get<ResourceType::FragmentShader>("model.fshader"_name))
                    .vertex_shader(resourceManager.get<ResourceType::VertexShader>("model.vshader"_name))
                    // Vertex description
                    .begin_vertex_layout<geometry::Vertex>()
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                    .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv))
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal))
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, tangent))
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, bitangent))
                    .end_vertex_layout()
                    // Descriptor layout
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::PipelineStage::VertexShader)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::PipelineStage::FragmentShader)
                    .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                        graphics_api::PipelineStage::FragmentShader)
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::PipelineStage::FragmentShader)
                    .enable_depth_test(true)
                    .enable_blending(false)
                    .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(100, 100, 100))),
    m_sampler(resourceManager.get<ResourceType::Sampler>("linear_repeat_mlod8_aniso.sampler"_name))
{
}

InstancedModel ModelRenderer::instance_model(const Name modelName, ShadowMap &shadowMap)
{
   const auto &model = m_resourceManager.get<ResourceType::Model>(modelName);
   auto descriptors  = checkResult(m_descriptorPool.allocate_array(model.range.size()));
   size_t index{};

   graphics_api::UniformBuffer<UniformBufferObject> ubo(m_device);
   graphics_api::UniformBuffer<MaterialProps> uboMatProps(m_device);

   for (const auto range : model.range) {
      const auto &material = m_resourceManager.get<ResourceType::Material>(range.materialName);
      const auto &texture  = m_resourceManager.get<ResourceType::Texture>(material.texture);

      graphics_api::DescriptorWriter descWriter(m_device, descriptors[index]);
      descWriter.set_uniform_buffer(0, ubo);
      descWriter.set_sampled_texture(1, texture, m_sampler);

      if (material.normal_texture != Name{}) {
         const auto &normalTexture = m_resourceManager.get<ResourceType::Texture>(material.normal_texture);
         descWriter.set_sampled_texture(2, normalTexture, m_sampler);
      }

      *uboMatProps = material.props;
      descWriter.set_uniform_buffer(3, uboMatProps);

      ++index;
   }

   return InstancedModel{modelName,
                         model.boudingBox,
                         glm::vec3{},
                         std::move(ubo),
                         std::move(uboMatProps),
                         std::move(descriptors),
                         shadowMap.create_model_properties()};
}

void ModelRenderer::set_active_command_list(graphics_api::CommandList *commandList)
{
   m_commandList = commandList;
}

void ModelRenderer::begin_render()
{
   m_commandList->bind_pipeline(m_pipeline);
}

void ModelRenderer::draw_model(const InstancedModel &instancedModel) const
{
   assert(m_commandList != nullptr);

   const auto &model = m_resourceManager.get<ResourceType::Model>(instancedModel.modelName);

   m_commandList->bind_vertex_array(model.mesh.vertices);
   m_commandList->bind_index_array(model.mesh.indices);

   size_t index{};
   for (const auto &range : model.range) {
      const auto descriptorSet = instancedModel.descriptors[index];

      m_commandList->bind_descriptor_set(descriptorSet);
      m_commandList->draw_indexed_primitives(static_cast<int>(range.size), static_cast<int>(range.offset), 0);
      ++index;
   }
}

graphics_api::CommandList &ModelRenderer::command_list() const
{
   return *m_commandList;
}

}// namespace renderer