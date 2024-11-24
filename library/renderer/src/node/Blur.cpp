#include "Blur.hpp"

#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"

namespace triglav::renderer::node {

namespace gapi = graphics_api;
using namespace name_literals;

static constexpr u32 upper_div(const u32 nom, const u32 denom)
{
   if ((nom % denom) == 0)
      return nom / denom;
   return 1 + (nom / denom);
}

BlurResources::BlurResources(gapi::Device& device, const bool isSingleChannel) :
    m_device(device),
    m_isSingleChannel(isSingleChannel)
{
}

void BlurResources::update_resolution(const gapi::Resolution& resolution)
{
   using gapi::TextureState;
   using gapi::TextureUsage;

   NodeFrameResources::update_resolution(resolution);

   m_outputTexture.emplace(GAPI_CHECK(m_device.create_texture(m_isSingleChannel ? GAPI_FORMAT(R, Float16) : GAPI_FORMAT(RGBA, Float16),
                                                              resolution, TextureUsage::Sampled | TextureUsage::Storage)));
   m_initialLayout = true;
}

gapi::Texture& BlurResources::texture()
{
   assert(m_outputTexture.has_value());
   return *m_outputTexture;
}

gapi::TextureState BlurResources::texture_state()
{
   if (m_initialLayout) {
      m_initialLayout = false;
      return graphics_api::TextureState::Undefined;
   }
   return graphics_api::TextureState::ShaderRead;
}

Blur::Blur(gapi::Device& device, resource::ResourceManager& resourceManager, const Name srcNode, const Name srcTexture,
           const bool isSingleChannel) :
    m_device(device),
    m_pipeline(GAPI_CHECK(graphics_api::ComputePipelineBuilder(m_device)
                             .compute_shader(resourceManager.get(isSingleChannel ? "blur/sc.cshader"_rc : "blur.cshader"_rc))
                             .descriptor_binding(gapi::DescriptorType::ImageSampler)
                             .descriptor_binding(gapi::DescriptorType::StorageImage)
                             .use_push_descriptors(true)
                             .build())),
    m_srcNode(srcNode),
    m_srcTexture(srcTexture),
    m_isSingleChannel(isSingleChannel)
{
}

std::unique_ptr<render_core::NodeFrameResources> Blur::create_node_resources()
{
   return std::make_unique<BlurResources>(m_device, m_isSingleChannel);
}

gapi::WorkTypeFlags Blur::work_types() const
{
   return gapi::WorkType::Graphics | gapi::WorkType::Compute;
}

void Blur::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                           gapi::CommandList& cmdList)
{
   const gapi::Texture& inTexture = frameResources.node(m_srcNode).texture(m_srcTexture);

   auto& blurResources = dynamic_cast<BlurResources&>(resources);
   gapi::Texture& outTexture = blurResources.texture();

   gapi::TextureBarrierInfo dstBarrierIn{
      .texture = &outTexture,
      .sourceState = blurResources.texture_state(),
      .targetState = gapi::TextureState::General,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(gapi::PipelineStage::FragmentShader, gapi::PipelineStage::ComputeShader, dstBarrierIn);

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_texture(0, inTexture);
   cmdList.bind_storage_image(1, outTexture);
   cmdList.dispatch(upper_div(inTexture.width(), 16), upper_div(inTexture.height(), 16), 1);

   gapi::TextureBarrierInfo dstBarrierOut{
      .texture = &outTexture,
      .sourceState = gapi::TextureState::General,
      .targetState = gapi::TextureState::ShaderRead,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(gapi::PipelineStage::ComputeShader, gapi::PipelineStage::FragmentShader, dstBarrierOut);
}

void Blur::set_source_texture(const Name srcNode, const Name srcTexture)
{
   m_srcNode = srcNode;
   m_srcTexture = srcTexture;
}

}// namespace triglav::renderer::node