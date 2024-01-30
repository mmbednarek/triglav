#pragma once

#include "graphics_api/Device.h"
#include "graphics_api/Pipeline.h"
#include "graphics_api/Texture.h"

#include "ResourceManager.h"

namespace renderer {

class PostProcessingRenderer
{
 public:
   struct PushConstant
   {
      alignas(16) glm::vec3 lightPosition{};
      int enableSSAO{1};
   };

   struct UniformData
   {
      glm::mat4 shadowMapMat;
   };

   PostProcessingRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                          const ResourceManager &resourceManager, const graphics_api::Texture &colorTexture,
                          const graphics_api::Texture &positionTexture,
                          const graphics_api::Texture &normalTexture, const graphics_api::Texture &aoTexture,
                          const graphics_api::Texture &shadowMapTexture);

   void update_textures(const graphics_api::Texture &colorTexture,
                        const graphics_api::Texture &positionTexture,
                        const graphics_api::Texture &normalTexture, const graphics_api::Texture &aoTexture,
                        const graphics_api::Texture &shadowMapTexture) const;

   void draw(graphics_api::CommandList &cmdList, const glm::vec3 &lightPosition,
             const glm::mat4 &shadowMapMat, bool ssaoEnabled) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler m_sampler;
   graphics_api::DescriptorArray m_descriptors;
   graphics_api::UniformBuffer<UniformData> m_uniformBuffer;
};

}// namespace renderer