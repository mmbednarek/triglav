#include "RayTracedImage.hpp"

namespace triglav::renderer::node {

using namespace name_literals;

using graphics_api::PipelineStage;
using graphics_api::TextureBarrierInfo;
using graphics_api::TextureState;
using graphics_api::TextureUsage;
using graphics_api::WorkType;

// RayTracedImageResources

RayTracedImageResources::RayTracedImageResources(graphics_api::Device& device) :
    m_device(device),
    m_texture(GAPI_CHECK(device.create_texture(GAPI_FORMAT(R, Float16), {1600, 900}, TextureUsage::Sampled | TextureUsage::Storage)))
{
   this->register_texture("ao"_name, m_texture);
}

const graphics_api::Texture& RayTracedImageResources::texture() const
{
   return m_texture;
}

TextureState RayTracedImageResources::in_texture_state()
{
   const auto res = m_initialLayout ? TextureState::Undefined : TextureState::ShaderRead;
   m_initialLayout = false;
   return res;
}

void RayTracedImageResources::update_resolution(const graphics_api::Resolution& resolution)
{
   NodeFrameResources::update_resolution(resolution);
   m_texture = GAPI_CHECK(m_device.create_texture(GAPI_FORMAT(R, Float16), {resolution.width, resolution.height},
                                                  TextureUsage::Sampled | TextureUsage::Storage));
   this->register_texture("ao"_name, m_texture);
}

// RayTracedImage

RayTracedImage::RayTracedImage(graphics_api::Device& device, RayTracingScene& rtScene, Scene& scene) :
    m_device(device),
    m_rtScene(rtScene),
    m_scene(scene),
    m_timestampArray(GAPI_CHECK(device.create_timestamp_array(2)))
{
}

graphics_api::WorkTypeFlags RayTracedImage::work_types() const
{
   return WorkType::Compute;
}

void RayTracedImage::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& nodeResources,
                                     graphics_api::CommandList& cmdList)
{
   cmdList.reset_timestamp_array(m_timestampArray, 0, 2);
   cmdList.write_timestamp(PipelineStage::Entrypoint, m_timestampArray, 0);

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

   m_rtScene.render(cmdList, texture);

   TextureBarrierInfo dstBarrierOut{
      .texture = &texture,
      .sourceState = TextureState::General,
      .targetState = TextureState::ShaderRead,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::RayGenerationShader, PipelineStage::ComputeShader, dstBarrierOut);

   cmdList.write_timestamp(PipelineStage::End, m_timestampArray, 1);
}

std::unique_ptr<render_core::NodeFrameResources> RayTracedImage::create_node_resources()
{
   return std::make_unique<RayTracedImageResources>(m_device);
}

float RayTracedImage::gpu_time() const
{
   return m_timestampArray.get_difference(0, 1);
}

}// namespace triglav::renderer::node
