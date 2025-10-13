#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <string>

namespace triglav::graphics_api {
DECLARE_VLK_WRAPPED_CHILD_OBJECT(ShaderModule, Device)

class Shader
{
 public:
   Shader(std::string name, PipelineStage stage, vulkan::ShaderModule module);

   [[nodiscard]] const vulkan::ShaderModule& vulkan_module() const;
   [[nodiscard]] std::string_view name() const;
   [[nodiscard]] PipelineStage stage() const;

   void set_debug_name(std::string_view name);

 private:
   std::string m_name;
   PipelineStage m_stage;
   vulkan::ShaderModule m_module;
};

}// namespace triglav::graphics_api
