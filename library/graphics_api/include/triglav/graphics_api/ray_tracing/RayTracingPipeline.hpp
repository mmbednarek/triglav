#pragma once

#include "../PipelineBuilder.hpp"

#include "triglav/Name.hpp"

#include <map>
#include <vector>

namespace triglav::graphics_api::ray_tracing {

struct ShaderIndexWithType
{
   Index shaderIndex;
   PipelineStage shaderType;
};

class RayTracingPipelineBuilder : public PipelineBuilderBase
{
 public:
   explicit RayTracingPipelineBuilder(Device& device);

   RayTracingPipelineBuilder& ray_generation_shader(Name name, const Shader& shader);
   RayTracingPipelineBuilder& any_hit_shader(Name name, const Shader& shader);
   RayTracingPipelineBuilder& closest_hit_shader(Name name, const Shader& shader);
   RayTracingPipelineBuilder& miss_shader(Name name, const Shader& shader);
   RayTracingPipelineBuilder& intersection_shader(Name name, const Shader& shader);
   RayTracingPipelineBuilder& callable_shader(Name name, const Shader& shader);

   RayTracingPipelineBuilder& descriptor_binding(PipelineStageFlags shaderStages, DescriptorType descriptorType);
   RayTracingPipelineBuilder& push_constant(PipelineStageFlags shaderStages, size_t size, size_t offset = 0);
   RayTracingPipelineBuilder& use_push_descriptors(bool enabled);

   RayTracingPipelineBuilder& max_recursion(u32 count);

   RayTracingPipelineBuilder& general_group(Name generalShader);

   template<typename... TArgs>
   RayTracingPipelineBuilder& triangle_group(Name generalShader, TArgs&&... args)
   {
      std::array<Name, sizeof...(args)> argsArr{args...};
      return this->shader_group_internal(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR, generalShader, argsArr);
   }

   Result<Pipeline> build();

 private:
   RayTracingPipelineBuilder& shader(Name name, const Shader& shader);
   RayTracingPipelineBuilder& shader_group_internal(VkRayTracingShaderGroupTypeKHR groupType, Name generalShader, std::span<Name> shaders);

   std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;
   std::map<Name, ShaderIndexWithType> m_shaderIndices;
   u32 m_maxRecursion{2};
};

}// namespace triglav::graphics_api::ray_tracing