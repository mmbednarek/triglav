#include "DescriptorArray.hpp"

#include "vulkan/Util.hpp"

#include <utility>

namespace triglav::graphics_api {

DescriptorArray::DescriptorArray(const VkDevice device, const VkDescriptorPool descriptor_pool,
                                 std::vector<VkDescriptorSet> descriptor_sets) :
    m_device(device),
    m_descriptor_pool(descriptor_pool),
    m_descriptor_sets(std::move(descriptor_sets))
{
}

DescriptorArray::DescriptorArray(DescriptorArray&& other) noexcept :
    m_device(std::exchange(other.m_device, nullptr)),
    m_descriptor_pool(std::exchange(other.m_descriptor_pool, nullptr)),
    m_descriptor_sets(std::move(other.m_descriptor_sets))
{
}

DescriptorArray& DescriptorArray::operator=(DescriptorArray&& other) noexcept
{
   m_device = std::exchange(other.m_device, nullptr);
   m_descriptor_pool = std::exchange(other.m_descriptor_pool, nullptr);
   m_descriptor_sets = std::move(other.m_descriptor_sets);
   return *this;
}

DescriptorArray::~DescriptorArray()
{
   if (not m_descriptor_sets.empty()) {
      vkFreeDescriptorSets(m_device, m_descriptor_pool, static_cast<u32>(m_descriptor_sets.size()), m_descriptor_sets.data());
   }
}

size_t DescriptorArray::count() const
{
   return m_descriptor_sets.size();
}

DescriptorView DescriptorArray::at(const size_t index) const
{
   return {m_descriptor_sets.at(index)};
}

DescriptorView DescriptorArray::operator[](const size_t index) const
{
   return this->at(index);
}

}// namespace triglav::graphics_api