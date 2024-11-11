#include "TimestampArray.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace triglav::graphics_api {

TimestampArray::TimestampArray(vulkan::QueryPool queryPool, const float timestampPeriod) :
    m_queryPool(std::move(queryPool)),
    m_timestampPeriod(timestampPeriod)
{
}

void TimestampArray::get_result(std::span<float> out, const u32 first) const
{
   std::vector<u64> timestamps{};
   timestamps.resize(out.size());
   vkGetQueryPoolResults(m_queryPool.parent(), *m_queryPool, first, timestamps.size(), sizeof(u64) * timestamps.size(), timestamps.data(),
                         sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

   std::ranges::transform(timestamps, out.begin(),
                          [this](const u64 timestamp) { return static_cast<float>(timestamp) * m_timestampPeriod; });
}

float TimestampArray::get_difference(const u32 begin, const u32 end) const
{
   std::vector<u64> timestamps{};
   timestamps.resize(1 + end - begin);
   vkGetQueryPoolResults(m_queryPool.parent(), *m_queryPool, begin, timestamps.size(), sizeof(u64) * timestamps.size(), timestamps.data(),
                         sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

   return static_cast<float>(timestamps[timestamps.size() - 1] - timestamps[0]) * m_timestampPeriod / 1000000.0f;
}

VkQueryPool TimestampArray::vulkan_query_pool() const
{
   return *m_queryPool;
}

}// namespace triglav::graphics_api
