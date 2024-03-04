#include "Shader.h"

namespace triglav::graphics_api
{
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
}
