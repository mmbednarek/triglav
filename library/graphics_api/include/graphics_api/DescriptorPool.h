#pragma once

#include "vulkan/ObjectWrapper.hpp"

#include "DescriptorArray.h"
#include "GraphicsApi.hpp"

namespace graphics_api
{

DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorPool, Device)

class DescriptorPool
{
public:
   DescriptorPool(vulkan::DescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout pipelineLayout);

   [[nodiscard]] Result<DescriptorArray> allocate_array(size_t descriptorCount);
private:
   vulkan::DescriptorPool m_descriptorPool;
   VkDescriptorSetLayout m_descriptorSetLayout;
   VkPipelineLayout m_pipelineLayout;
};

}