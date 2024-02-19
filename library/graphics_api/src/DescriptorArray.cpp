#include "DescriptorArray.h"

#include "vulkan/Util.h"

#include <utility>

namespace triglav::graphics_api {

DescriptorArray::DescriptorArray(const VkDevice device, const VkDescriptorPool descriptorPool,
                                 const VkPipelineLayout pipelineLayout,
                                 std::vector<VkDescriptorSet> descriptorSets) :
    m_device(device),
    m_descriptorPool(descriptorPool),
    m_pipelineLayout(pipelineLayout),
    m_descriptorSets(std::move(descriptorSets))
{
}

DescriptorArray::DescriptorArray(DescriptorArray &&other) noexcept :
    m_device(std::exchange(other.m_device, nullptr)),
    m_descriptorPool(std::exchange(other.m_descriptorPool, nullptr)),
    m_pipelineLayout(std::exchange(other.m_pipelineLayout, nullptr)),
    m_descriptorSets(std::move(other.m_descriptorSets))
{
}

DescriptorArray &DescriptorArray::operator=(DescriptorArray &&other) noexcept
{
   m_device         = std::exchange(other.m_device, nullptr);
   m_descriptorPool = std::exchange(other.m_descriptorPool, nullptr);
   m_pipelineLayout = std::exchange(other.m_pipelineLayout, nullptr);
   m_descriptorSets = std::move(other.m_descriptorSets);
   return *this;
}

DescriptorArray::~DescriptorArray()
{
   if (not m_descriptorSets.empty()) {
      vkFreeDescriptorSets(m_device, m_descriptorPool, m_descriptorSets.size(), m_descriptorSets.data());
   }
}

size_t DescriptorArray::count() const
{
   return m_descriptorSets.size();
}

VkPipelineLayout DescriptorArray::pipeline_layout() const
{
   return m_pipelineLayout;
}

DescriptorView DescriptorArray::at(const size_t index) const
{
   return {m_descriptorSets.at(index), m_pipelineLayout};
}

DescriptorView DescriptorArray::operator[](const size_t index) const
{
   return this->at(index);
}

}// namespace graphics_api