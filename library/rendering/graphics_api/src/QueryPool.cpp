#include "QueryPool.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace triglav::graphics_api {

QueryPool::QueryPool(vulkan::QueryPool query_pool, const float timestamp_period) :
    m_query_pool(std::move(query_pool)),
    m_timestamp_period(timestamp_period)
{
}

u32 QueryPool::get_int(const u32 index)
{
   u32 result{};
   vkGetQueryPoolResults(m_query_pool.parent(), *m_query_pool, index, 1, sizeof(u32), &result, sizeof(u32), VK_QUERY_RESULT_WAIT_BIT);
   return result;
}

void QueryPool::get_result(std::span<float> out, const u32 first) const
{
   std::vector<u64> timestamps{};
   timestamps.resize(out.size());
   vkGetQueryPoolResults(m_query_pool.parent(), *m_query_pool, first, static_cast<u32>(timestamps.size()), sizeof(u64) * timestamps.size(),
                         timestamps.data(), sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

   std::ranges::transform(timestamps, out.begin(),
                          [this](const u64 timestamp) { return static_cast<float>(timestamp) * m_timestamp_period; });
}

float QueryPool::get_difference(const u32 begin, const u32 end) const
{
   std::vector<u64> timestamps{};
   timestamps.resize(1 + end - begin);
   vkGetQueryPoolResults(m_query_pool.parent(), *m_query_pool, begin, static_cast<u32>(timestamps.size()), sizeof(u64) * timestamps.size(),
                         timestamps.data(), sizeof(u64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

   return static_cast<float>(timestamps[timestamps.size() - 1] - timestamps[0]) * m_timestamp_period / 1000000.0f;
}

VkQueryPool QueryPool::vulkan_query_pool() const
{
   return *m_query_pool;
}

}// namespace triglav::graphics_api
