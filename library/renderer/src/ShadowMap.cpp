#include "ShadowMap.h"

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"

#include "ModelRenderer.h"
#include "Core.h"
#include "ResourceManager.h"

namespace renderer {

constexpr auto g_shadowMapResolution = graphics_api::Resolution{4096, 4096};
constexpr auto g_shadowMapFormat     = GAPI_FORMAT(D, Float32);

ShadowMap::ShadowMap(graphics_api::Device &device, ResourceManager &resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_depthRenderTarget(
            checkResult(device.create_depth_render_target(GAPI_FORMAT(D, Float32), g_shadowMapResolution))),
    m_depthTexture(checkResult(device.create_texture(g_shadowMapFormat, g_shadowMapResolution, graphics_api::TextureType::SampledDepthBuffer))),
    m_renderPass(checkResult(device.create_render_pass(m_depthRenderTarget))),
    m_framebuffer(checkResult(m_depthRenderTarget.create_framebuffer(m_renderPass, m_depthTexture))),
    m_pipeline(checkResult(
            graphics_api::PipelineBuilder(device, m_renderPass)
                    .fragment_shader(resourceManager.shader("fsh:shadow_map"_name))
                    .vertex_shader(resourceManager.shader("vsh:shadow_map"_name))
                    .begin_vertex_layout<geometry::Vertex>()
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                    .end_vertex_layout()
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::ShaderStage::Vertex)
                    .enable_depth_test(true)
                    .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(100, 1, 100)))
{
}

const graphics_api::Texture &ShadowMap::depth_texture() const
{
   return m_depthTexture;
}

const graphics_api::Framebuffer &ShadowMap::framebuffer() const
{
   return m_framebuffer;
}

void ShadowMap::on_begin_render(const ModelRenderer &ctx) const
{
   ctx.command_list().bind_pipeline(m_pipeline);
}

void ShadowMap::draw_model(const ModelRenderer &ctx, const InstancedModel &instancedModel) const
{
   const auto &model = m_resourceManager.model(instancedModel.modelName);
   const auto &mesh  = m_resourceManager.mesh(model.meshName);

   ctx.command_list().bind_vertex_array(mesh.vertices);
   ctx.command_list().bind_index_array(mesh.indices);

   const auto firstOffset = model.range[0].offset;
   size_t size{};
   for (const auto &range : model.range) {
      size += range.size;
   }

   const auto descriptorSet = instancedModel.shadowMap.descriptors[0];
   ctx.command_list().bind_descriptor_set(descriptorSet);
   ctx.command_list().draw_indexed_primitives(size, firstOffset, 0);
}

ModelShaderMapProperties ShadowMap::create_model_properties()
{
   auto shadowMapDescriptors = checkResult(m_descriptorPool.allocate_array(1));

   graphics_api::UniformBuffer<ShadowMapUBO> shadowMapUbo(m_device);

   graphics_api::DescriptorWriter smDescWriter(m_device, shadowMapDescriptors[0]);
   smDescWriter.set_uniform_buffer(0, shadowMapUbo);

   return ModelShaderMapProperties{std::move(shadowMapUbo), std::move(shadowMapDescriptors)};
}

}// namespace renderer