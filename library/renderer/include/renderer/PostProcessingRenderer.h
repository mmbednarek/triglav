#pragma once

#include "graphics_api/Device.h"
#include "graphics_api/Pipeline.h"
#include "graphics_api/Texture.h"

#include "ResourceManager.h"

namespace renderer {

class PostProcessingRenderer
{
 public:
   struct PushConstants
   {
      int enableFXAA{};
   };

   PostProcessingRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                          const ResourceManager &resourceManager, const graphics_api::Texture &colorTexture);

   void update_texture(const graphics_api::Texture &colorTexture) const;

   void draw(graphics_api::CommandList &cmdList, bool enableFXAA) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler m_sampler;
   graphics_api::DescriptorArray m_descriptors;
};

}// namespace renderer