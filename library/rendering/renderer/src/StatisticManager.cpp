#include "StatisticManager.hpp"

namespace triglav::renderer {

void StatisticManager::initialize()
{
   m_last_accumulation_point = std::chrono::steady_clock::now();
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
   auto diff = now - m_last_accumulation_point;
   if (diff < 500ms)
      return;

   m_last_accumulation_point = now;

   for (auto& prop : m_properties) {
      std::unique_lock lk{prop.mtx};

      if (prop.samples == 0)
         continue;

      prop.total_accumulated += prop.accumulated;
      prop.total_samples += static_cast<float>(prop.samples);
      prop.total_average = prop.total_accumulated / prop.total_samples;

      prop.value = prop.accumulated / static_cast<float>(prop.samples);
      prop.accumulated = 0.0f;
      prop.samples = 0;

      if (prop.max_value < prop.value)
         prop.max_value = prop.value;
      if (prop.min_value > prop.value)
         prop.min_value = prop.value;
   }
}

float StatisticManager::value(Stat stat) const
{
   return m_properties[static_cast<u32>(stat)].value;
}

float StatisticManager::min(Stat stat) const
{
   auto& props = m_properties[static_cast<u32>(stat)];
   if (props.total_samples <= 0.0f) {
      return 0.0f;
   }
   return m_properties[static_cast<u32>(stat)].min_value;
}

float StatisticManager::max(Stat stat) const
{
   auto& props = m_properties[static_cast<u32>(stat)];
   if (props.total_samples <= 0.0f) {
      return 0.0f;
   }
   return m_properties[static_cast<u32>(stat)].max_value;
}
float StatisticManager::average(Stat stat) const
{
   return m_properties[static_cast<u32>(stat)].total_average;
}

StatisticManager& StatisticManager::the()
{
   static StatisticManager instance;
   return instance;
}

}// namespace triglav::renderer
