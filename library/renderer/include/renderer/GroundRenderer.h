#pragma once

#include "Camera.h"
#include "ResourceManager.h"

#include "graphics_api/Device.h"
#include "graphics_api/HostVisibleBuffer.hpp"
#include "graphics_api/Pipeline.h"

#include <glm/mat4x4.hpp>

namespace renderer {

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
                  const ResourceManager &resourceManager);

   void draw(graphics_api::CommandList &cmdList, const Camera &camera) const;

 private:
   graphics_api::Device &m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::DescriptorPool m_descriptorPool;
   graphics_api::DescriptorArray m_descriptors;
   graphics_api::UniformBuffer<UniformData> m_uniformBuffer;
};

}// namespace renderer
