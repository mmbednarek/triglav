#pragma once
#include "GraphicsApi.hpp"

#include <vulkan/vulkan.h>

namespace graphics_api::vulkan {
Result<VkFormat> to_vulkan_color_format(const ColorFormat &format);
Result<ColorFormat> to_color_format(VkFormat format);

Result<VkColorSpaceKHR> to_vulkan_color_space(ColorSpace colorSpace);
Result<ColorSpace> to_color_space(VkColorSpaceKHR colorSpace);


VkShaderStageFlagBits to_vulkan_shader_stage(ShaderStage stage);
VkShaderStageFlags to_vulkan_shader_stage_flags(ShaderStageFlags flags);
VkDescriptorType to_vulkan_descriptor_type(DescriptorType descriptorType);
}// namespace graphics_api::vulkan
