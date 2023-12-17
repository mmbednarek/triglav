#include "DescriptorView.h"

#include "vulkan/Util.h"

namespace graphics_api {

DescriptorView::DescriptorView(const VkDescriptorSet descriptorSet,
                               const VkPipelineLayout pipelineLayout) :
    m_descriptorSet(descriptorSet),
    m_pipelineLayout(pipelineLayout)
{
}

VkDescriptorSet DescriptorView::vulkan_descriptor_set() const
{
   return m_descriptorSet;
}

VkPipelineLayout DescriptorView::vulkan_pipeline_layout() const
{
   return m_pipelineLayout;
}


}// namespace graphics_api
