#pragma once

#include "Camera.h"

#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/graphics_api/ReplicatedBuffer.hpp"
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
   using UniformBuffer = graphics_api::UniformReplicatedBuffer<UniformData>;

   GroundRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget, resource::ResourceManager& resourceManager);

   static void prepare_resources(graphics_api::CommandList& cmdList, UniformBuffer& ubo, const Camera& camera);
   void draw(graphics_api::CommandList& cmdList, UniformBuffer& ubo) const;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Texture& m_texture;
};

}// namespace triglav::renderer
