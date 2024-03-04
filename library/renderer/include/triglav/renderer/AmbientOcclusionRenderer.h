#pragma once

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/resource/ResourceManager.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace triglav::renderer {

constexpr size_t g_SampleCountSSAO = 64;

class AmbientOcclusionRenderer
{
 public:
   struct AlignedVec3
   {
      alignas(16) glm::vec3 value{};
   };

   struct UniformData
   {
      glm::mat4 cameraProjection;
      AlignedVec3 samplesSSAO[g_SampleCountSSAO];
   };

   AmbientOcclusionRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                            triglav::resource::ResourceManager &resourceManager,
                            const graphics_api::Texture &positionTexture,
                            const graphics_api::Texture &normalTexture,
                            const graphics_api::Texture &noiseTexture);

   void update_textures(const graphics_api::Texture &positionTexture,
                        const graphics_api::Texture &normalTexture,
                        const graphics_api::Texture &noiseTexture) const;

   void draw(graphics_api::CommandList &cmdList, const glm::mat4 &cameraProjection) const;
   static std::vector<AlignedVec3> generate_sample_points(size_t count);

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::Sampler& m_sampler;
   graphics_api::DescriptorArray m_descriptors;
   std::vector<AlignedVec3> m_samplesSSAO;
   graphics_api::UniformBuffer<UniformData> m_uniformBuffer;
};

}// namespace renderer
