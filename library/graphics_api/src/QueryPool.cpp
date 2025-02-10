#include "QueryPool.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace triglav::graphics_api {

QueryPool::QueryPool(vulkan::QueryPool queryPool, const float timestampPeriod) :
    m_queryPool(std::move(queryPool)),
    m_timestampPeriod(timestampPeriod)
{
}

u32 QueryPool::get_int(const u32 index)
{
   u32 result{};
   vkGetQueryPoolResults(m_queryPool.parent(), *m_queryPool, index, 1, sizeof(u32), &result, sizeof(u32), VK_QUERY_RESULT_WAIT_BIT);
   return result;
}

void QueryPool::get_result(std::span<float> out, const u32 first) const
{
   std::vector<u64> timestamps{};
   timestamps.resize(out.size());
   vkGetQueryPoolResults(m_queryPool.parent(), *m_queryPool, first, timestamps.size(), sizeof(u64) * timestamps.size(), timestamps.data(),
                         sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

   std::ranges::transform(timestamps, out.begin(),
                          [this](const u64 timestamp) { return static_cast<float>(timestamp) * m_timestampPeriod; });
}

float QueryPool::get_difference(const u32 begin, const u32 end) const
{
   std::vector<u64> timestamps{};
   timestamps.resize(1 + end - begin);
   vkGetQueryPoolResults(m_queryPool.parent(), *m_queryPool, begin, timestamps.size(), sizeof(u64) * timestamps.size(), timestamps.data(),
                         sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

   return static_cast<float>(timestamps[timestamps.size() - 1] - timestamps[0]) * m_timestampPeriod / 1000000.0f;
}

VkQueryPool QueryPool::vulkan_query_pool() const
{
   return *m_queryPool;
}

}// namespace triglav::graphics_api
