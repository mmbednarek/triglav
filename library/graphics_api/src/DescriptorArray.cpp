#include "DescriptorArray.hpp"

#include "vulkan/Util.hpp"

#include <utility>

namespace triglav::graphics_api {

DescriptorArray::DescriptorArray(const VkDevice device, const VkDescriptorPool descriptorPool,
                                 std::vector<VkDescriptorSet> descriptorSets) :
    m_device(device),
    m_descriptorPool(descriptorPool),
    m_descriptorSets(std::move(descriptorSets))
{
}

DescriptorArray::DescriptorArray(DescriptorArray&& other) noexcept :
    m_device(std::exchange(other.m_device, nullptr)),
    m_descriptorPool(std::exchange(other.m_descriptorPool, nullptr)),
    m_descriptorSets(std::move(other.m_descriptorSets))
{
}

DescriptorArray& DescriptorArray::operator=(DescriptorArray&& other) noexcept
{
   m_device = std::exchange(other.m_device, nullptr);
   m_descriptorPool = std::exchange(other.m_descriptorPool, nullptr);
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

DescriptorView DescriptorArray::at(const size_t index) const
{
   return {m_descriptorSets.at(index)};
}

DescriptorView DescriptorArray::operator[](const size_t index) const
{
   return this->at(index);
}

}// namespace triglav::graphics_api