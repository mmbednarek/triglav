#include "ray_tracing/RayTracingPipeline.hpp"

#include "Device.hpp"
#include "Shader.hpp"
#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api::ray_tracing {

RayTracingPipelineBuilder::RayTracingPipelineBuilder(Device& device) :
    PipelineBuilderBase(device)
{
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::ray_generation_shader(Name name, const Shader& shader)
{
   assert(shader.stage() == PipelineStage::RayGenerationShader);
   return this->shader(name, shader);
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::any_hit_shader(Name name, const Shader& shader)
{
   assert(shader.stage() == PipelineStage::AnyHitShader);
   return this->shader(name, shader);
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::closest_hit_shader(Name name, const Shader& shader)
{
   assert(shader.stage() == PipelineStage::ClosestHitShader);
   return this->shader(name, shader);
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::miss_shader(Name name, const Shader& shader)
{
   assert(shader.stage() == PipelineStage::MissShader);
   return this->shader(name, shader);
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::intersection_shader(Name name, const Shader& shader)
{
   assert(shader.stage() == PipelineStage::IntersectionShader);
   return this->shader(name, shader);
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::callable_shader(Name name, const Shader& shader)
{
   return this->shader(name, shader);
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::descriptor_binding(PipelineStageFlags shader_stages, DescriptorType descriptor_type)
{
   this->add_descriptor_binding(descriptor_type, shader_stages, 1);
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::push_constant(PipelineStageFlags shader_stages, size_t size, size_t offset)
{
   this->add_push_constant(shader_stages, size, offset);
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::use_push_descriptors(bool enabled)
{
   m_use_push_descriptors = enabled;
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::max_recursion(u32 count)
{
   m_max_recursion = count;
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::shader(const Name name, const Shader& shader)
{
   auto index = this->add_shader(shader);
   m_shader_indices.emplace(name, ShaderIndexWithType{.shader_index = static_cast<u32>(index), .shader_type = shader.stage()});
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::general_group(const Name general_shader)
{
   return this->shader_group_internal(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, std::span<const Name>{&general_shader, 1});
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::shader_group_internal(const VkRayTracingShaderGroupTypeKHR group_type,
                                                                            std::span<const Name> shaders)
{
   VkRayTracingShaderGroupCreateInfoKHR info{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
   info.type = group_type;

   info.generalShader = VK_SHADER_UNUSED_KHR;
   info.intersectionShader = VK_SHADER_UNUSED_KHR;
   info.closestHitShader = VK_SHADER_UNUSED_KHR;
   info.anyHitShader = VK_SHADER_UNUSED_KHR;

   for (const auto& shader_name : shaders) {
      auto [index, stage] = m_shader_indices.at(shader_name);
      switch (stage) {
      case PipelineStage::RayGenerationShader:
         [[fallthrough]];
      case PipelineStage::MissShader:
         info.generalShader = index;
         break;
      case PipelineStage::AnyHitShader:
         info.anyHitShader = index;
         break;
      case PipelineStage::ClosestHitShader:
         info.closestHitShader = index;
         break;
      case PipelineStage::IntersectionShader:
         info.intersectionShader = index;
         break;
      default:
         continue;
      }
   }

   m_shader_groups.push_back(info);

   return *this;
}

Result<RayTracingPipeline> RayTracingPipelineBuilder::build()
{
   VkRayTracingPipelineCreateInfoKHR pipeline_create_info{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
   pipeline_create_info.stageCount = static_cast<u32>(m_shader_stage_infos.size());
   pipeline_create_info.pStages = m_shader_stage_infos.data();
   pipeline_create_info.groupCount = static_cast<u32>(m_shader_groups.size());
   pipeline_create_info.pGroups = m_shader_groups.data();
   pipeline_create_info.maxPipelineRayRecursionDepth = m_max_recursion;

   auto pipeline_layout = this->build_pipeline_layout();
   if (not pipeline_layout.has_value()) {
      return std::unexpected{pipeline_layout.error()};
   }

   auto&& [desc_set_layout, layout] = *pipeline_layout;
   pipeline_create_info.layout = *layout;

   VkPipeline vulkan_pipeline{};
   VkResult result = vulkan::vkCreateRayTracingPipelinesKHR(m_device.vulkan_device(), nullptr, nullptr, 1, &pipeline_create_info, nullptr,
                                                            &vulkan_pipeline);
   if (result != VK_SUCCESS) {
      return std::unexpected{Status::PSOCreationFailed};
   }

   vulkan::Pipeline pipeline(m_device.vulkan_device());
   pipeline.take_ownership(vulkan_pipeline);

   return RayTracingPipeline{std::move(layout),        std::move(pipeline), std::move(desc_set_layout),
                             PipelineType::RayTracing, m_shader_indices,    static_cast<u32>(m_shader_groups.size())};
}

RayTracingPipeline::RayTracingPipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline,
                                       vulkan::DescriptorSetLayout descriptor_set_layout, PipelineType pipeline_type,
                                       std::map<Name, ShaderIndexWithType> indices, u32 group_count) :
    Pipeline(std::move(layout), std::move(pipeline), std::move(descriptor_set_layout), pipeline_type),
    m_shader_indices(std::move(indices)),
    m_group_count(group_count)
{
}

u32 RayTracingPipeline::group_count() const
{
   return m_group_count;
}

u32 RayTracingPipeline::shader_count() const
{
   return static_cast<u32>(m_shader_indices.size());
}

ShaderIndexWithType RayTracingPipeline::shader_index(Name name) const
{
   return m_shader_indices.at(name);
}

}// namespace triglav::graphics_api::ray_tracing
