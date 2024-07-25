#include "ShadingRenderer.h"

#include <cstring>
#include <random>

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/Framebuffer.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"

#include "node/Blur.h"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

ShadingRenderer::ShadingRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget, ResourceManager& resourceManager) :
    m_device(device),
    m_pipeline(
       checkResult(graphics_api::GraphicsPipelineBuilder(m_device, renderTarget)
                      .fragment_shader(resourceManager.get("shading.fshader"_rc))
                      .vertex_shader(resourceManager.get("shading.vshader"_rc))
                      .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                      .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                      .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                      .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                      .descriptor_binding_array(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader, 3)
                      .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::FragmentShader)
                      .push_constant(graphics_api::PipelineStage::FragmentShader, sizeof(PushConstant))
                      .use_push_descriptors(true)
                      .enable_depth_test(false)
                      .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                      .build()))
{
}

void ShadingRenderer::draw(render_core::FrameResources& resources, graphics_api::CommandList& cmdList, const glm::vec3& lightPosition,
                           const std::array<glm::mat4, 3>& shadowMapMats, const glm::mat4& viewMat,
                           graphics_api::UniformBuffer<UniformData>& ubo)
{
   for (u32 i = 0; i < shadowMapMats.size(); ++i) {
      ubo->shadowMapMats[i] = shadowMapMats[i];
   }
   ubo->viewMat = viewMat;

   cmdList.bind_pipeline(m_pipeline);

   auto& gbuffer = resources.node("geometry"_name).framebuffer("gbuffer"_name);
   auto& aoTexture = dynamic_cast<node::BlurResources&>(resources.node("blur_ao"_name)).texture();
   auto& smBuffer1 = resources.node("shadow_map"_name).framebuffer("sm1"_name);
   auto& smBuffer2 = resources.node("shadow_map"_name).framebuffer("sm2"_name);
   auto& smBuffer3 = resources.node("shadow_map"_name).framebuffer("sm3"_name);

   std::array<graphics_api::Texture*, 3> shadowMaps{&smBuffer1.texture("sm"_name), &smBuffer2.texture("sm"_name),
                                                    &smBuffer3.texture("sm"_name)};

   PushConstant pushConstant{
      .lightPosition = lightPosition,
      .enableSSAO = resources.has_flag("ssao"_name),
   };
   cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, pushConstant);

   cmdList.bind_texture(0, gbuffer.texture("albedo"_name));
   cmdList.bind_texture(1, gbuffer.texture("position"_name));
   cmdList.bind_texture(2, gbuffer.texture("normal"_name));
   cmdList.bind_texture(3, aoTexture);
   cmdList.bind_texture_array(4, shadowMaps);
   cmdList.bind_uniform_buffer(5, ubo);

   cmdList.draw_primitives(4, 0);
}

}// namespace triglav::renderer