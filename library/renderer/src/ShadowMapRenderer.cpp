#include "ShadowMapRenderer.h"

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"
#include "triglav/resource/ResourceManager.h"

#include "ModelRenderer.h"

using triglav::ResourceType;
using triglav::graphics_api::AttachmentAttribute;
using triglav::render_core::checkResult;
using triglav::render_core::InstancedModel;
using triglav::render_core::ModelShaderMapProperties;
using triglav::render_core::ShadowMapUBO;
using triglav::resource::ResourceManager;

using namespace triglav::name_literals;

namespace triglav::renderer {

ShadowMapRenderer::ShadowMapRenderer(graphics_api::Device &device, ResourceManager &resourceManager, graphics_api::RenderTarget &depthRenderTarget) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_depthRenderTarget(depthRenderTarget),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(device, m_depthRenderTarget)
                    .fragment_shader(
                            resourceManager.get<ResourceType::FragmentShader>("shadow_map.fshader"_name))
                    .vertex_shader(resourceManager.get<ResourceType::VertexShader>("shadow_map.vshader"_name))
                    .begin_vertex_layout<geometry::Vertex>()
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                    .end_vertex_layout()
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::PipelineStage::VertexShader)
                    .enable_depth_test(true)
                    .use_push_descriptors(true)
                    .build()))
{
}

void ShadowMapRenderer::on_begin_render(graphics_api::CommandList &cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);
}

void ShadowMapRenderer::draw_model(graphics_api::CommandList &cmdList,
                                   const InstancedModel &instancedModel) const
{
   const auto &model = m_resourceManager.get<ResourceType::Model>(instancedModel.modelName);

   cmdList.bind_vertex_array(model.mesh.vertices);
   cmdList.bind_index_array(model.mesh.indices);

   const auto firstOffset = model.range[0].offset;
   size_t size{};
   for (const auto &range : model.range) {
      size += range.size;
   }

   graphics_api::DescriptorWriter smDescWriter(m_device);
   smDescWriter.set_uniform_buffer(0, instancedModel.shadowMap.ubo);
   cmdList.push_descriptors(0, smDescWriter);

   cmdList.draw_indexed_primitives(size, firstOffset, 0);
}

ModelShaderMapProperties ShadowMapRenderer::create_model_properties()
{
   graphics_api::UniformBuffer<ShadowMapUBO> shadowMapUbo(m_device);
   return ModelShaderMapProperties{std::move(shadowMapUbo)};
}

}// namespace triglav::renderer