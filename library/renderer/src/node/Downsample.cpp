#include "Downsample.hpp"

#include "triglav/graphics_api/Device.hpp"

namespace triglav::renderer::node {

using namespace name_literals;

using graphics_api::CommandList;
using graphics_api::Device;
using graphics_api::PipelineStage;
using graphics_api::Resolution;
using graphics_api::Texture;
using graphics_api::TextureBarrierInfo;
using graphics_api::TextureRegion;
using graphics_api::TextureState;
using graphics_api::TextureUsage;
using graphics_api::WorkType;
using render_core::FrameResources;
using render_core::NodeFrameResources;

DownsampleResources::DownsampleResources(Device& device) :
    m_device(device)
{
}

void DownsampleResources::update_resolution(const Resolution& resolution)
{
   m_downsampledTexture.emplace(GAPI_CHECK(m_device.create_texture(
      GAPI_FORMAT(RGBA, Float16), {resolution.width / 2, resolution.height / 2}, TextureUsage::TransferDst | TextureUsage::Sampled)));
}

graphics_api::Texture& DownsampleResources::texture()
{
   assert(m_downsampledTexture.has_value());
   return *m_downsampledTexture;
}

Downsample::Downsample(Device& device, Name srcNode, Name srcFramebuffer, Name srcAttachment) :
    m_device(device),
    m_srcNode(srcNode),
    m_srcFramebuffer(srcFramebuffer),
    m_srcAttachment(srcAttachment)
{
}

std::unique_ptr<render_core::NodeFrameResources> Downsample::create_node_resources()
{
   return std::make_unique<DownsampleResources>(m_device);
}

void Downsample::record_commands(FrameResources& frameResources, NodeFrameResources& resources, CommandList& cmdList)
{
   auto& downsampleResources = dynamic_cast<DownsampleResources&>(resources);

   auto& framebuffer = frameResources.node(m_srcNode).framebuffer(m_srcFramebuffer);
   auto& srcTexture = framebuffer.texture(m_srcAttachment);

   const TextureBarrierInfo barrierSrcIn{
      .texture = &srcTexture,
      .sourceState = TextureState::ShaderRead,
      .targetState = TextureState::TransferSrc,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::FragmentShader, PipelineStage::Transfer, barrierSrcIn);

   const TextureBarrierInfo barrierDstIn{
      .texture = &downsampleResources.texture(),
      .sourceState = TextureState::Undefined,
      .targetState = TextureState::TransferDst,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::Transfer, barrierDstIn);


   TextureRegion srcRegion{
      .offsetMin = {0, 0},
      .offsetMax = {srcTexture.resolution().width, srcTexture.resolution().height},
      .mipLevel = 0,
   };
   TextureRegion dstRegion{
      .offsetMin = {0, 0},
      .offsetMax = {srcTexture.resolution().width / 2, srcTexture.resolution().height / 2},
      .mipLevel = 0,
   };
   cmdList.blit_texture(srcTexture, srcRegion, downsampleResources.texture(), dstRegion);

   const TextureBarrierInfo barrierDstOut{
      .texture = &downsampleResources.texture(),
      .sourceState = TextureState::TransferDst,
      .targetState = TextureState::ShaderRead,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrierDstOut);

   const TextureBarrierInfo barrierSrcOut{
      .texture = &srcTexture,
      .sourceState = TextureState::TransferSrc,
      .targetState = TextureState::ShaderRead,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrierSrcOut);
}

graphics_api::WorkTypeFlags Downsample::work_types() const
{
   return WorkType::Graphics;
}

}// namespace triglav::renderer::node
