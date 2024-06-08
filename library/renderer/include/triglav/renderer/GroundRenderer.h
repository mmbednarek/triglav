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
 public:
   struct UniformData
   {
      glm::mat4 model;
      glm::mat4 view;
      glm::mat4 proj;
   };
   using UniformBuffer = graphics_api::UniformBuffer<UniformData>;

   GroundRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget, resource::ResourceManager& resourceManager);

   void draw(graphics_api::CommandList& cmdList, UniformBuffer& ubo, const Camera& camera) const;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Texture& m_texture;
};

}// namespace triglav::renderer
