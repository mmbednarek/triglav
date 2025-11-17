#include "DescriptorView.hpp"

namespace triglav::graphics_api {

DescriptorView::DescriptorView(const VkDescriptorSet descriptor_set) :
    m_descriptor_set(descriptor_set)
{
}

VkDescriptorSet DescriptorView::vulkan_descriptor_set() const
{
   return m_descriptor_set;
}

}// namespace triglav::graphics_api
