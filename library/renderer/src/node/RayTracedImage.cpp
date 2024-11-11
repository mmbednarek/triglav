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
    m_ambientOcclusionTexture(
       GAPI_CHECK(device.create_texture(GAPI_FORMAT(R, Float16), {1600, 900}, TextureUsage::Sampled | TextureUsage::Storage))),
    m_shadowsTexture(GAPI_CHECK(device.create_texture(GAPI_FORMAT(R, Float16), {1600, 900}, TextureUsage::Sampled | TextureUsage::Storage)))
{
   this->register_texture("ao"_name, m_ambientOcclusionTexture);
   this->register_texture("shadows"_name, m_shadowsTexture);
}

const graphics_api::Texture& RayTracedImageResources::ao_texture() const
{
   return m_ambientOcclusionTexture;
}
const graphics_api::Texture& RayTracedImageResources::shadows_texture() const
{
   return m_shadowsTexture;
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
   m_ambientOcclusionTexture = GAPI_CHECK(m_device.create_texture(GAPI_FORMAT(R, Float16), {resolution.width, resolution.height},
                                                                  TextureUsage::Sampled | TextureUsage::Storage));
   m_shadowsTexture = GAPI_CHECK(m_device.create_texture(GAPI_FORMAT(R, Float16), {resolution.width, resolution.height},
                                                         TextureUsage::Sampled | TextureUsage::Storage));
   this->register_texture("ao"_name, m_ambientOcclusionTexture);
   this->register_texture("shadows"_name, m_shadowsTexture);
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
   if (!frameResources.has_flag("ray_traced_shadows"_name) && frameResources.get_option<AmbientOcclusionMethod>("ao_method"_name) != AmbientOcclusionMethod::RayTraced)
      return;

   cmdList.reset_timestamp_array(m_timestampArray, 0, 2);
   cmdList.write_timestamp(PipelineStage::Entrypoint, m_timestampArray, 0);

   auto& res = dynamic_cast<RayTracedImageResources&>(nodeResources);
   const auto texState = res.in_texture_state();
   auto& aoTexture = res.ao_texture();
   auto& shadowsTexture = res.shadows_texture();

   std::array dstBarriersIn{
      TextureBarrierInfo{
         .texture = &aoTexture,
         .sourceState = texState,
         .targetState = TextureState::General,
         .baseMipLevel = 0,
         .mipLevelCount = 1,
      },
      TextureBarrierInfo{
         .texture = &shadowsTexture,
         .sourceState = texState,
         .targetState = TextureState::General,
         .baseMipLevel = 0,
         .mipLevelCount = 1,
      },
   };
   cmdList.texture_barrier(PipelineStage::ComputeShader, PipelineStage::RayGenerationShader, dstBarriersIn);

   m_rtScene.render(cmdList, aoTexture, shadowsTexture);

   std::array dstBarriersOut{
      TextureBarrierInfo {
         .texture = &aoTexture,
         .sourceState = TextureState::General,
         .targetState = TextureState::ShaderRead,
         .baseMipLevel = 0,
         .mipLevelCount = 1,
      },
      TextureBarrierInfo {
         .texture = &shadowsTexture,
         .sourceState = TextureState::General,
         .targetState = TextureState::ShaderRead,
         .baseMipLevel = 0,
         .mipLevelCount = 1,
      },
   };
   cmdList.texture_barrier(PipelineStage::RayGenerationShader, PipelineStage::ComputeShader, dstBarriersOut);

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
