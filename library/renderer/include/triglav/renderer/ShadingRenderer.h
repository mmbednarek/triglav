#pragma once

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.h"
#include "triglav/graphics_api/Texture.h"
#include "triglav/render_core/FrameResources.h"
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

   ShadingRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget, resource::ResourceManager& resourceManager);

   void draw(render_core::FrameResources& resources, graphics_api::CommandList& cmdList, const glm::vec3& lightPosition,
             const glm::mat4& shadowMapMat) const;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::UniformBuffer<UniformData> m_uniformBuffer;
};

}// namespace triglav::renderer
