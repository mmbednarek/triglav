#pragma once

#include "vulkan/ObjectWrapper.hpp"

#include "triglav/Int.hpp"

#include <span>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(QueryPool, Device)

class QueryPool
{
 public:
   QueryPool(vulkan::QueryPool query_pool, float timestamp_period);

   u32 get_int(u32 index);

   void get_result(std::span<float> out, u32 first) const;
   [[nodiscard]] float get_difference(u32 begin, u32 end) const;
   [[nodiscard]] VkQueryPool vulkan_query_pool() const;

 private:
   vulkan::QueryPool m_query_pool;
   float m_timestamp_period;
};

}// namespace triglav::graphics_api
