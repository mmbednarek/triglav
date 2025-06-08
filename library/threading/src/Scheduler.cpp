#include "Scheduler.hpp"

namespace triglav::threading {

TimerHandle Scheduler::register_timeout(const Duration duration, std::function<void()> func)
{
   TimerHandle handle = m_topHandle++;
   m_callbacks.emplace(handle, std::move(func));
   m_timers.emplace(std::chrono::steady_clock::now() + duration, handle);
   return handle;
}

void Scheduler::cancel(const TimerHandle handle)
{
   m_callbacks.erase(handle);
   std::erase_if(m_timers, [handle](const auto& pair) { return pair.second == handle; });
}

void Scheduler::tick()
{
   while (!m_callbacks.empty()) {
      const auto callback = m_timers.begin();
      if (callback->first <= std::chrono::steady_clock::now()) {
         auto handle = callback->second;
         m_callbacks.at(handle)();
         m_callbacks.erase(handle);
         m_timers.erase(callback);
      } else {
         return;
      }
   }
}

Scheduler& Scheduler::the()
{
   static Scheduler scheduler;
   return scheduler;
}

}// namespace triglav::threading
