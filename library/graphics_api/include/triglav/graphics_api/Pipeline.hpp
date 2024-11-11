#pragma once

#include "DescriptorPool.hpp"
#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(PipelineLayout, Device)

class Buffer;
class Texture;

namespace vulkan {
using Pipeline = WrappedObject<VkPipeline, vkCreateGraphicsPipelines, vkDestroyPipeline, VkDevice>;
}

class Pipeline
{
 public:
   Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline, vulkan::DescriptorSetLayout descriptorSetLayout,
            PipelineType pipelineType);

   [[nodiscard]] VkPipeline vulkan_pipeline() const;
   [[nodiscard]] const vulkan::PipelineLayout& layout() const;
   [[nodiscard]] Result<DescriptorPool> create_descriptor_pool(uint32_t uniformBufferCount, uint32_t sampledImageCount,
                                                               uint32_t maxDescriptorCount);
   [[nodiscard]] PipelineType pipeline_type() const;

 private:
   vulkan::PipelineLayout m_layout;
   vulkan::Pipeline m_pipeline;
   vulkan::DescriptorSetLayout m_descriptorSetLayout;
   PipelineType m_pipelineType;
};

}// namespace triglav::graphics_api
