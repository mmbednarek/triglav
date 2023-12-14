#include "Pipeline.h"

#include "vulkan/Util.h"

namespace graphics_api {

Pipeline::Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline,
                   vulkan::DescriptorSetLayout descriptorSetLayout) :
    m_layout(std::move(layout)),
    m_pipeline(std::move(pipeline)),
    m_descriptorSetLayout(std::move(descriptorSetLayout))
{
}

VkPipeline Pipeline::vulkan_pipeline() const
{
   return *m_pipeline;
}

const vulkan::PipelineLayout &Pipeline::layout() const
{
   return m_layout;
}

Result<DescriptorPool> Pipeline::create_descriptor_pool(const uint32_t uniformBufferCount,
                                                        const uint32_t sampledImageCount,
                                                        const uint32_t maxDescriptorCount)
{
   const std::array descriptorPoolSizes{
           VkDescriptorPoolSize{
                                .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                .descriptorCount = uniformBufferCount,
                                },
           VkDescriptorPoolSize{
                                .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                .descriptorCount = sampledImageCount,
                                },
   };

   VkDescriptorPoolCreateInfo descriptorPoolInfo{};
   descriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   descriptorPoolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
   descriptorPoolInfo.poolSizeCount = descriptorPoolSizes.size();
   descriptorPoolInfo.pPoolSizes    = descriptorPoolSizes.data();
   descriptorPoolInfo.maxSets       = maxDescriptorCount;

   vulkan::DescriptorPool descriptorPool{m_pipeline.parent()};
   if (descriptorPool.construct(&descriptorPoolInfo) != VK_SUCCESS)
      return std::unexpected(Status::UnsupportedDevice);

   return DescriptorPool(std::move(descriptorPool), *m_descriptorSetLayout, *m_layout);
}

}// namespace graphics_api
