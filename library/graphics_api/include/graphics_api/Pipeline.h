#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"
#include "DescriptorGroup.h"

#include <variant>

namespace graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(PipelineLayout, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorPool, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorSetLayout, Device)

class Buffer;
class Texture;

namespace vulkan {
using Pipeline = WrappedObject<VkPipeline, vkCreateGraphicsPipelines, vkDestroyPipeline, VkDevice>;
}

class Pipeline
{
 public:
   Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline, vulkan::DescriptorPool descriptorPool,
            vulkan::DescriptorSetLayout descriptorSetLayout, VkSampler sampler, uint32_t framebufferCount);

   [[nodiscard]] VkPipeline vulkan_pipeline() const;
   [[nodiscard]] const vulkan::PipelineLayout &layout() const;
   [[nodiscard]] Result<DescriptorGroup> allocate_descriptors();

 private:
   vulkan::PipelineLayout m_layout;
   vulkan::Pipeline m_pipeline;
   vulkan::DescriptorPool m_descriptorPool;
   vulkan::DescriptorSetLayout m_descriptorSetLayout;
   VkSampler m_sampler;
   uint32_t m_framebufferCount{};
};

}// namespace graphics_api
