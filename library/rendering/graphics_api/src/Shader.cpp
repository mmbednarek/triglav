#include "Shader.hpp"

#include "vulkan/DynamicProcedures.hpp"

namespace triglav::graphics_api {
Shader::Shader(std::string name, const PipelineStage stage, vulkan::ShaderModule module) :
    m_name(std::move(name)),
    m_stage(stage),
    m_module(std::move(module))
{
}

const vulkan::ShaderModule& Shader::vulkan_module() const
{
   return m_module;
}

std::string_view Shader::name() const
{
   return m_name;
}

PipelineStage Shader::stage() const
{
   return m_stage;
}

void Shader::set_debug_name(const std::string_view name)
{
   VkDebugUtilsObjectNameInfoEXT debug_utils_object_name{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
   debug_utils_object_name.objectHandle = reinterpret_cast<u64>(*m_module);
   debug_utils_object_name.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
   debug_utils_object_name.pObjectName = name.data();
   [[maybe_unused]] const auto result = vulkan::vkSetDebugUtilsObjectNameEXT(m_module.parent(), &debug_utils_object_name);
   assert(result == VK_SUCCESS);
}

}// namespace triglav::graphics_api
