#include "RayTracedImage.hpp"

namespace triglav::renderer::node {

using graphics_api::TextureUsage;
using graphics_api::TextureState;
using graphics_api::WorkType;
using graphics_api::TextureBarrierInfo;
using graphics_api::PipelineStage;

// RayTracedImageResources

RayTracedImageResources::RayTracedImageResources(graphics_api::Device& device) :
   m_texture(GAPI_CHECK(device.create_texture(GAPI_FORMAT(RGBA, Float16), {1600, 900}, TextureUsage::Sampled | TextureUsage::Storage)))
{
}

const graphics_api::Texture& RayTracedImageResources::texture() const
{
   return m_texture;
}

graphics_api::TextureState RayTracedImageResources::in_texture_state()
{
   const auto res = m_initialLayout ? TextureState::Undefined : TextureState::ShaderRead;
   m_initialLayout = false;
   return res;
}

// RayTracedImage

RayTracedImage::RayTracedImage(graphics_api::Device& device, RayTracingScene& rtScene, Scene& scene) :
    m_device(device),
    m_rtScene(rtScene),
    m_scene(scene)
{
}

graphics_api::WorkTypeFlags RayTracedImage::work_types() const
{
   return WorkType::Compute;
}

void RayTracedImage::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& nodeResources,
                                     graphics_api::CommandList& cmdList)
{
   auto& res = dynamic_cast<RayTracedImageResources&>(nodeResources);
   auto& texture = res.texture();

   TextureBarrierInfo dstBarrierIn{
      .texture = &texture,
      .sourceState = res.in_texture_state(),
      .targetState = TextureState::General,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::ComputeShader, PipelineStage::RayGenerationShader, dstBarrierIn);

   m_rtScene.render(cmdList, texture, m_scene.camera());

   TextureBarrierInfo dstBarrierOut{
      .texture = &texture,
      .sourceState = TextureState::General,
      .targetState = TextureState::ShaderRead,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::RayGenerationShader, PipelineStage::ComputeShader, dstBarrierOut);
}

std::unique_ptr<render_core::NodeFrameResources> RayTracedImage::create_node_resources()
{
   return std::make_unique<RayTracedImageResources>(m_device);
}

}// namespace triglav::renderer::node
