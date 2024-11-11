#pragma once

#include "vulkan/ObjectWrapper.hpp"

#include "triglav/Int.hpp"

#include <span>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(QueryPool, Device)

class TimestampArray
{
 public:
   TimestampArray(vulkan::QueryPool queryPool, float timestampPeriod);

   void get_result(std::span<float> out, u32 first) const;
   [[nodiscard]] float get_difference(u32 begin, u32 end) const;
   [[nodiscard]] VkQueryPool vulkan_query_pool() const;

 private:
   vulkan::QueryPool m_queryPool;
   float m_timestampPeriod;
};

}// namespace triglav::graphics_api
