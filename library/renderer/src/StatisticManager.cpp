#include "StatisticManager.h"

namespace triglav::renderer {

void StatisticManager::initialize()
{
   m_lastAccumulationPoint = std::chrono::steady_clock::now();
}

void StatisticManager::push_accumulated(Stat stat, float value)
{
   auto& props = m_properties[static_cast<u32>(stat)];
   std::unique_lock lk{props.mtx};

   props.accumulated += value;
   ++props.samples;
}

void StatisticManager::tick()
{
   using namespace std::chrono_literals;

   const auto now = std::chrono::steady_clock::now();
   auto diff = now - m_lastAccumulationPoint;
   if (diff < 500ms)
      return;

   m_lastAccumulationPoint = now;

   for (auto& prop : m_properties) {
      std::unique_lock lk{prop.mtx};

      if (prop.samples == 0)
         continue;

      prop.totalAccumulated += prop.accumulated;
      prop.totalSamples += static_cast<float>(prop.samples);
      prop.totalAverage = prop.totalAccumulated / prop.totalSamples;

      prop.value = prop.accumulated / static_cast<float>(prop.samples);
      prop.accumulated = 0.0f;
      prop.samples = 0;

      if (prop.maxValue < prop.value)
         prop.maxValue = prop.value;
      if (prop.minValue > prop.value)
         prop.minValue = prop.value;
   }
}

float StatisticManager::value(Stat stat) const
{
   return m_properties[static_cast<u32>(stat)].value;
}

float StatisticManager::min(Stat stat) const
{
   auto& props = m_properties[static_cast<u32>(stat)];
   if (props.totalSamples <= 0.0f) {
      return 0.0f;
   }
   return m_properties[static_cast<u32>(stat)].minValue;
}

float StatisticManager::max(Stat stat) const
{
   auto& props = m_properties[static_cast<u32>(stat)];
   if (props.totalSamples <= 0.0f) {
      return 0.0f;
   }
   return m_properties[static_cast<u32>(stat)].maxValue;
}
float StatisticManager::average(Stat stat) const
{
   return m_properties[static_cast<u32>(stat)].totalAverage;
}

StatisticManager& StatisticManager::the()
{
   static StatisticManager instance;
   return instance;
}

}// namespace triglav::renderer
