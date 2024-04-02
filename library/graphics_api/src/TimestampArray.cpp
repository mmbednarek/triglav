#include "TimestampArray.h"

#include <utility>

namespace triglav::graphics_api {

TimestampArray::TimestampArray(vulkan::QueryPool queryPool, const float timestampPeriod) :
    m_queryPool(std::move(queryPool)),
    m_timestampPeriod(timestampPeriod)
{
}

void TimestampArray::get_result(std::span<u64> out, const u32 first) const
{
   vkGetQueryPoolResults(m_queryPool.parent(), *m_queryPool, first, out.size(), sizeof(u64) * out.size(),
                         out.data(), sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

   for (auto &value : out) {
      value *= m_timestampPeriod;
   }
}

VkQueryPool TimestampArray::vulkan_query_pool() const
{
   return *m_queryPool;
}

}// namespace triglav::graphics_api
