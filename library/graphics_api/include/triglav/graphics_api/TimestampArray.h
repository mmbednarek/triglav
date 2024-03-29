#pragma once

#include "vulkan/ObjectWrapper.hpp"

#include "triglav/Int.hpp"

#include <span>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(QueryPool, Device)

class TimestampArray {
public:
  TimestampArray(vulkan::QueryPool queryPool, u64 timestampPeriod);

  [[nodiscard]] void get_result(std::span<u64> out, u32 first) const;
  [[nodiscard]] VkQueryPool vulkan_query_pool() const;
private:
  vulkan::QueryPool m_queryPool;
  u64 m_timestampPeriod;
};

}
