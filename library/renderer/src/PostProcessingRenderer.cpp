#include "PostProcessingRenderer.h"

#include "src/node/Blur.h"
#include "src/node/Downsample.h"
#include "triglav/graphics_api/CommandList.hpp"
#include "triglav/graphics_api/DescriptorWriter.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

PostProcessingRenderer::PostProcessingRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget,
                                               ResourceManager& resourceManager) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::GraphicsPipelineBuilder(m_device, renderTarget)
                              .fragment_shader(resourceManager.get("post_processing.fshader"_rc))
                              .vertex_shader(resourceManager.get("post_processing.vshader"_rc))
                              // Descriptor layout
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                              .enable_depth_test(false)
                              .use_push_descriptors(true)
                              .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                              .push_constant(graphics_api::PipelineStage::FragmentShader, sizeof(PushConstants))
                              .build()))
{
}

void PostProcessingRenderer::draw(render_core::FrameResources& resources, graphics_api::CommandList& cmdList) const
{
   cmdList.bind_pipeline(m_pipeline);

   auto& shading = resources.node("shading"_name).framebuffer("shading"_name);
   auto& ui = resources.node("user_interface"_name).framebuffer("ui"_name);
   auto& bloomTexture = dynamic_cast<node::BlurResources&>(resources.node("blur_bloom"_name)).texture();

   cmdList.bind_texture(0, shading.texture("shading"_name));
   cmdList.bind_texture(1, bloomTexture);
   cmdList.bind_texture(2, ui.texture("user_interface"_name));

   PushConstants constants{
      .enableFXAA = resources.has_flag("fxaa"_name),
      .hideUI = resources.has_flag("hide_ui"_name),
      .bloomEnabled = resources.has_flag("bloom"_name),
   };
   cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, constants);
   cmdList.draw_primitives(4, 0);
}

}// namespace triglav::renderer