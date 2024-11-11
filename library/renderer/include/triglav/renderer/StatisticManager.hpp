#pragma once

#include "triglav/Int.hpp"
#include "triglav/Name.hpp"

#include <array>
#include <chrono>
#include <mutex>
#include <numeric>

namespace triglav::renderer {

struct StatProperties
{
   float value{.0f};
   float accumulated{.0f};
   float minValue{std::numeric_limits<float>::max()};
   float maxValue{std::numeric_limits<float>::lowest()};
   float totalAverage{0.0f};
   float totalAccumulated{0.0f};
   float totalSamples{0.0f};
   u32 samples{0};

   std::mutex mtx;
};

enum class Stat : u32
{
   FramesPerSecond,
   GBufferGpuTime,
   ShadingGpuTime,
   RayTracingGpuTime,
   Count
};

class StatisticManager
{
 public:
   void initialize();
   void push_accumulated(Stat stat, float value);
   void tick();

   [[nodiscard]] float value(Stat stat) const;
   [[nodiscard]] float min(Stat stat) const;
   [[nodiscard]] float max(Stat stat) const;
   [[nodiscard]] float average(Stat stat) const;

   [[nodiscard]] static StatisticManager& the();

 private:
   std::array<StatProperties, static_cast<u32>(Stat::Count)> m_properties;
   std::chrono::steady_clock::time_point m_lastAccumulationPoint;
};

}// namespace triglav::renderer
