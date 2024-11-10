#pragma once

#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/HostVisibleBuffer.hpp"
#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/render_core/FrameResources.h"
#include "triglav/resource/ResourceManager.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace triglav::renderer {

constexpr size_t g_SampleCountSSAO = 64;

enum class AmbientOcclusionMethod
{
   None,
   ScreenSpace,
   RayTraced,
};

inline std::string_view ambient_occlusion_method_to_string(const AmbientOcclusionMethod method)
{
   switch (method) {
   case AmbientOcclusionMethod::None:
      return "None";
   case AmbientOcclusionMethod::ScreenSpace:
      return "Screen Space";
   case AmbientOcclusionMethod::RayTraced:
      return "Ray Traced";
   }
}

class AmbientOcclusionRenderer
{
 public:
   struct AlignedVec3
   {
      alignas(16) glm::vec3 value{};
   };

   struct UniformData
   {
      glm::mat4 cameraProjection{};
      AlignedVec3 samplesSSAO[g_SampleCountSSAO];
   };

   AmbientOcclusionRenderer(graphics_api::Device& device, graphics_api::RenderTarget& renderTarget,
                            resource::ResourceManager& resourceManager, const graphics_api::Texture& noiseTexture);

   void draw(render_core::FrameResources& resources, graphics_api::CommandList& cmdList, const glm::mat4& cameraProjection) const;
   static std::vector<AlignedVec3> generate_sample_points(size_t count);

 private:
   graphics_api::Device& m_device;
   graphics_api::Pipeline m_pipeline;
   std::vector<AlignedVec3> m_samplesSSAO;
   graphics_api::UniformBuffer<UniformData> m_uniformBuffer;
   const graphics_api::Texture& m_noiseTexture;
};

}// namespace triglav::renderer
