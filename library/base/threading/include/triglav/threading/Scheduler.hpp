#pragma once

#include "triglav/Int.hpp"

#include <chrono>
#include <functional>
#include <map>

namespace triglav::threading {

using TimerHandle = u32;

class Scheduler
{
 public:
   using Duration = std::chrono::steady_clock::duration;
   using TimePoint = std::chrono::steady_clock::time_point;

   TimerHandle register_timeout(Duration duration, std::function<void()> func);

   void cancel(TimerHandle handle);
   void tick();

   [[nodiscard]] static Scheduler& the();

 private:
   TimerHandle m_top_handle{};
   std::multimap<TimePoint, TimerHandle> m_timers;
   std::map<TimerHandle, std::function<void()>> m_callbacks;
};

}// namespace triglav::threading