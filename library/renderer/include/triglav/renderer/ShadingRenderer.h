#pragma once

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/graphics_api/Texture.h"
#include "triglav/resource/ResourceManager.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace triglav::renderer {

class ShadingRenderer
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

   ShadingRenderer(graphics_api::Device &device, graphics_api::RenderTarget &renderTarget,
                   resource::ResourceManager &resourceManager,
                   graphics_api::Framebuffer &geometryBuffer, const graphics_api::Texture &aoTexture,
                   const graphics_api::Texture &shadowMapTexture);

   void update_textures(graphics_api::Framebuffer &geometryBuffer, const graphics_api::Texture &aoTexture,
                        const graphics_api::Texture &shadowMapTexture) const;

   void draw(graphics_api::CommandList &cmdList, const glm::vec3 &lightPosition,
             const glm::mat4 &shadowMapMat, bool ssaoEnabled) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler& m_sampler;
   graphics_api::DescriptorArray m_descriptors;
   graphics_api::UniformBuffer<UniformData> m_uniformBuffer;
};

}// namespace renderer
