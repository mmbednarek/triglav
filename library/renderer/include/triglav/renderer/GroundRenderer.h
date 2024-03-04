#pragma once

#include "Camera.h"

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/resource/ResourceManager.h"

#include <glm/mat4x4.hpp>

namespace triglav::renderer {

class GroundRenderer
{
   struct UniformData
   {
      glm::mat4 model;
      glm::mat4 view;
      glm::mat4 proj;
   };

 public:
   GroundRenderer(graphics_api::Device &device, graphics_api::RenderPass &renderPass,
                  triglav::resource::ResourceManager &resourceManager);

   void draw(graphics_api::CommandList &cmdList, const Camera &camera) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::DescriptorArray m_descriptors;
   graphics_api::UniformBuffer<UniformData> m_uniformBuffer;
   graphics_api::Sampler& m_sampler;
};

}// namespace renderer
