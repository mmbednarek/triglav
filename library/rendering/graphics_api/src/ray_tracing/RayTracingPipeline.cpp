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

RayTracingPipelineBuilder& RayTracingPipelineBuilder::descriptor_binding(PipelineStageFlags shaderStages, DescriptorType descriptorType)
{
   this->add_descriptor_binding(descriptorType, shaderStages, 1);
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::push_constant(PipelineStageFlags shaderStages, size_t size, size_t offset)
{
   this->add_push_constant(shaderStages, size, offset);
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::use_push_descriptors(bool enabled)
{
   m_usePushDescriptors = enabled;
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::max_recursion(u32 count)
{
   m_maxRecursion = count;
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::shader(const Name name, const Shader& shader)
{
   auto index = this->add_shader(shader);
   m_shaderIndices.emplace(name, ShaderIndexWithType{.shaderIndex = static_cast<u32>(index), .shaderType = shader.stage()});
   return *this;
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::general_group(const Name generalShader)
{
   return this->shader_group_internal(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, std::span<const Name>{&generalShader, 1});
}

RayTracingPipelineBuilder& RayTracingPipelineBuilder::shader_group_internal(const VkRayTracingShaderGroupTypeKHR groupType,
                                                                            std::span<const Name> shaders)
{
   VkRayTracingShaderGroupCreateInfoKHR info{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
   info.type = groupType;

   info.generalShader = VK_SHADER_UNUSED_KHR;
   info.intersectionShader = VK_SHADER_UNUSED_KHR;
   info.closestHitShader = VK_SHADER_UNUSED_KHR;
   info.anyHitShader = VK_SHADER_UNUSED_KHR;

   for (const auto& shaderName : shaders) {
      auto [index, stage] = m_shaderIndices.at(shaderName);
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

   m_shaderGroups.push_back(info);

   return *this;
}

Result<RayTracingPipeline> RayTracingPipelineBuilder::build()
{
   VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
   pipelineCreateInfo.stageCount = static_cast<u32>(m_shaderStageInfos.size());
   pipelineCreateInfo.pStages = m_shaderStageInfos.data();
   pipelineCreateInfo.groupCount = static_cast<u32>(m_shaderGroups.size());
   pipelineCreateInfo.pGroups = m_shaderGroups.data();
   pipelineCreateInfo.maxPipelineRayRecursionDepth = m_maxRecursion;

   auto pipelineLayout = this->build_pipeline_layout();
   if (not pipelineLayout.has_value()) {
      return std::unexpected{pipelineLayout.error()};
   }

   auto&& [descSetLayout, layout] = *pipelineLayout;
   pipelineCreateInfo.layout = *layout;

   VkPipeline vulkanPipeline{};
   VkResult result =
      vulkan::vkCreateRayTracingPipelinesKHR(m_device.vulkan_device(), nullptr, nullptr, 1, &pipelineCreateInfo, nullptr, &vulkanPipeline);
   if (result != VK_SUCCESS) {
      return std::unexpected{Status::PSOCreationFailed};
   }

   vulkan::Pipeline pipeline(m_device.vulkan_device());
   pipeline.take_ownership(vulkanPipeline);

   return RayTracingPipeline{std::move(layout),        std::move(pipeline), std::move(descSetLayout),
                             PipelineType::RayTracing, m_shaderIndices,     static_cast<u32>(m_shaderGroups.size())};
}

RayTracingPipeline::RayTracingPipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline,
                                       vulkan::DescriptorSetLayout descriptorSetLayout, PipelineType pipelineType,
                                       std::map<Name, ShaderIndexWithType> indices, u32 groupCount) :
    Pipeline(std::move(layout), std::move(pipeline), std::move(descriptorSetLayout), pipelineType),
    m_shaderIndices(std::move(indices)),
    m_groupCount(groupCount)
{
}

u32 RayTracingPipeline::group_count() const
{
   return m_groupCount;
}

u32 RayTracingPipeline::shader_count() const
{
   return static_cast<u32>(m_shaderIndices.size());
}

ShaderIndexWithType RayTracingPipeline::shader_index(Name name) const
{
   return m_shaderIndices.at(name);
}

}// namespace triglav::graphics_api::ray_tracing
