#pragma once

#include "../PipelineBuilder.hpp"

#include "triglav/Name.hpp"

#include <map>
#include <vector>

namespace triglav::graphics_api::ray_tracing {

class RayTracingPipeline;

struct ShaderIndexWithType
{
   u32 shader_index;
   PipelineStage shader_type;
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

   RayTracingPipelineBuilder& descriptor_binding(PipelineStageFlags shader_stages, DescriptorType descriptor_type);
   RayTracingPipelineBuilder& push_constant(PipelineStageFlags shader_stages, size_t size, size_t offset = 0);
   RayTracingPipelineBuilder& use_push_descriptors(bool enabled);

   RayTracingPipelineBuilder& max_recursion(u32 count);

   RayTracingPipelineBuilder& general_group(Name general_shader);

   template<typename... TArgs>
   RayTracingPipelineBuilder& triangle_group(TArgs&&... args)
   {
      std::array<Name, sizeof...(args)> args_arr{args...};
      return this->shader_group_internal(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR, args_arr);
   }

   Result<RayTracingPipeline> build();

 private:
   RayTracingPipelineBuilder& shader(Name name, const Shader& shader);
   RayTracingPipelineBuilder& shader_group_internal(VkRayTracingShaderGroupTypeKHR group_type, std::span<const Name> shaders);

   std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shader_groups;
   std::map<Name, ShaderIndexWithType> m_shader_indices;
   u32 m_max_recursion{1};
};

class RayTracingPipeline : public Pipeline
{
 public:
   RayTracingPipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline, vulkan::DescriptorSetLayout descriptor_set_layout,
                      PipelineType pipeline_type, std::map<Name, ShaderIndexWithType> indices, u32 group_count);

   [[nodiscard]] u32 group_count() const;
   [[nodiscard]] u32 shader_count() const;
   [[nodiscard]] ShaderIndexWithType shader_index(Name name) const;

 private:
   std::map<Name, ShaderIndexWithType> m_shader_indices;
   u32 m_group_count{};
};

}// namespace triglav::graphics_api::ray_tracing