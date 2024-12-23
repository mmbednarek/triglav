#pragma once

#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/graphics_api/Texture.hpp"
#include "triglav/render_core/FrameResources.hpp"
#include "triglav/resource/ResourceManager.hpp"

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
      int shouldSampleShadows{false};
   };

   struct UniformData
   {
      glm::mat4 viewMat;
      glm::mat4 shadowMapMats[3];
   };

   ShadingRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget, resource::ResourceManager& resourceManager);

   void draw(render_core::FrameResources& resources, graphics_api::CommandList& cmdList, const glm::vec3& lightPosition,
             const std::array<glm::mat4, 3>& shadowMapMats, const glm::mat4& viewMat,
             const graphics_api::UniformBuffer<UniformData>& ubo) const;

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   graphics_api::Texture m_placeholderTexture;
};

}// namespace triglav::renderer
