#pragma once
#include "GraphicsApi.hpp"

#include <tuple>
#include <vulkan/vulkan.h>

namespace triglav::graphics_api::vulkan {
Result<VkFormat> to_vulkan_color_format(const ColorFormat& format);
Result<ColorFormat> to_color_format(VkFormat format);

Result<VkColorSpaceKHR> to_vulkan_color_space(ColorSpace colorSpace);
Result<ColorSpace> to_color_space(VkColorSpaceKHR colorSpace);

WorkTypeFlags vulkan_queue_flags_to_work_type_flags(VkQueueFlags flags, bool canPresent);

Result<VkSampleCountFlagBits> to_vulkan_sample_count(SampleCount sampleCount);
Result<VkImageLayout> to_vulkan_image_layout(AttachmentAttributeFlags type);
std::tuple<VkAttachmentLoadOp, VkAttachmentStoreOp> to_vulkan_load_store_ops(AttachmentAttributeFlags flags);


VkShaderStageFlagBits to_vulkan_shader_stage(PipelineStage stage);
VkShaderStageFlags to_vulkan_shader_stage_flags(PipelineStageFlags flags);
VkPipelineStageFlagBits to_vulkan_pipeline_stage(PipelineStage stage);
VkPipelineStageFlags to_vulkan_pipeline_stage_flags(PipelineStageFlags flags);
VkDescriptorType to_vulkan_descriptor_type(DescriptorType descriptorType);
VkImageLayout to_vulkan_image_layout(TextureState resourceState);
VkAccessFlags to_vulkan_access_flags(TextureState resourceState);
VkFilter to_vulkan_filter(FilterType filterType);
VkSamplerAddressMode to_vulkan_sampler_address_mode(TextureAddressMode mode);
VkImageUsageFlags to_vulkan_image_usage_flags(TextureUsageFlags usage);
VkImageAspectFlags to_vulkan_image_aspect_flags(TextureUsageFlags usage);
VkBufferUsageFlags to_vulkan_buffer_usage_flags(BufferUsageFlags usage);
VkMemoryPropertyFlags to_vulkan_memory_properties_flags(BufferUsageFlags usage);
VkPipelineStageFlags to_vulkan_wait_pipeline_stage(WorkTypeFlags workTypes);
VkPipelineBindPoint to_vulkan_pipeline_bind_point(PipelineType pipelineType);
VkImageAspectFlags to_vulkan_aspect_flags(TextureUsageFlags usageFlags);
VkPresentModeKHR to_vulkan_present_mode(PresentMode presentMode);
}// namespace triglav::graphics_api::vulkan
