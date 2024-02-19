#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <string>

namespace triglav::graphics_api {
DECLARE_VLK_WRAPPED_CHILD_OBJECT(ShaderModule, Device)

class Shader
{
 public:
   Shader(std::string name, ShaderStage stage, vulkan::ShaderModule module);

   [[nodiscard]] const vulkan::ShaderModule &vulkan_module() const;
   [[nodiscard]] std::string_view name() const;
   [[nodiscard]] ShaderStage stage() const;

 private:
   std::string m_name;
   ShaderStage m_stage;
   vulkan::ShaderModule m_module;
};

}// namespace graphics_api
