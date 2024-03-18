#include "ShadowMapRenderer.h"

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/resource/ResourceManager.h"
#include "triglav/render_core/RenderCore.hpp"

#include "ModelRenderer.h"

using triglav::resource::ResourceManager;
using triglav::ResourceType;
using triglav::render_core::InstancedModel;
using triglav::render_core::ModelShaderMapProperties;
using triglav::render_core::checkResult;
using triglav::render_core::ShadowMapUBO;

using namespace triglav::name_literals;

namespace triglav::renderer {

constexpr auto g_shadowMapResolution = graphics_api::Resolution{4096, 4096};
constexpr auto g_shadowMapFormat     = GAPI_FORMAT(D, Float32);

ShadowMapRenderer::ShadowMapRenderer(graphics_api::Device &device, ResourceManager &resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_depthRenderTarget(
            checkResult(device.create_depth_render_target(GAPI_FORMAT(D, Float32), g_shadowMapResolution))),
    m_depthTexture(checkResult(device.create_texture(g_shadowMapFormat, g_shadowMapResolution,
                                                     graphics_api::TextureType::SampledDepthBuffer))),
    m_renderPass(checkResult(device.create_render_pass(m_depthRenderTarget))),
    m_framebuffer(checkResult(m_depthRenderTarget.create_framebuffer(m_renderPass, m_depthTexture))),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(device, m_renderPass)
                    .fragment_shader(resourceManager.get<ResourceType::FragmentShader>("shadow_map.fshader"_name))
                    .vertex_shader(resourceManager.get<ResourceType::VertexShader>("shadow_map.vshader"_name))
                    .begin_vertex_layout<geometry::Vertex>()
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                    .end_vertex_layout()
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::PipelineStage::VertexShader)
                    .enable_depth_test(true)
                    .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(100, 1, 100)))
{
}

const graphics_api::Texture &ShadowMapRenderer::depth_texture() const
{
   return m_depthTexture;
}

const graphics_api::Framebuffer &ShadowMapRenderer::framebuffer() const
{
   return m_framebuffer;
}

void ShadowMapRenderer::on_begin_render(graphics_api::CommandList &cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);
}

void ShadowMapRenderer::draw_model(graphics_api::CommandList &cmdList, const InstancedModel &instancedModel) const
{
   const auto &model = m_resourceManager.get<ResourceType::Model>(instancedModel.modelName);

   cmdList.bind_vertex_array(model.mesh.vertices);
   cmdList.bind_index_array(model.mesh.indices);

   const auto firstOffset = model.range[0].offset;
   size_t size{};
   for (const auto &range : model.range) {
      size += range.size;
   }

   const auto descriptorSet = instancedModel.shadowMap.descriptors[0];
   cmdList.bind_descriptor_set(descriptorSet);
   cmdList.draw_indexed_primitives(size, firstOffset, 0);
}

ModelShaderMapProperties ShadowMapRenderer::create_model_properties()
{
   auto shadowMapDescriptors = checkResult(m_descriptorPool.allocate_array(1));

   graphics_api::UniformBuffer<ShadowMapUBO> shadowMapUbo(m_device);

   graphics_api::DescriptorWriter smDescWriter(m_device, shadowMapDescriptors[0]);
   smDescWriter.set_uniform_buffer(0, shadowMapUbo);

   return ModelShaderMapProperties{std::move(shadowMapUbo), std::move(shadowMapDescriptors)};
}

}// namespace renderer