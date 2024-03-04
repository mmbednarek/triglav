#pragma once

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/graphics_api/Texture.h"

#include "triglav/resource/ResourceManager.h"

namespace triglav::renderer {

class PostProcessingRenderer
{
 public:
   struct PushConstants
   {
      int enableFXAA{};
   };

   PostProcessingRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                          triglav::resource::ResourceManager &resourceManager,
                          const graphics_api::Texture &colorTexture,
                          const graphics_api::Texture &depthTexture);

   void update_texture(const graphics_api::Texture &colorTexture,
                       const graphics_api::Texture &depthTexture) const;

   void draw(graphics_api::CommandList &cmdList, bool enableFXAA) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler& m_sampler;
   graphics_api::DescriptorArray m_descriptors;
};

}// namespace renderer