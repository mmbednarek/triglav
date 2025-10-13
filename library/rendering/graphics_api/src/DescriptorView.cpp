#include "DescriptorView.hpp"

namespace triglav::graphics_api {

DescriptorView::DescriptorView(const VkDescriptorSet descriptorSet) :
    m_descriptorSet(descriptorSet)
{
}

VkDescriptorSet DescriptorView::vulkan_descriptor_set() const
{
   return m_descriptorSet;
}

}// namespace triglav::graphics_api
