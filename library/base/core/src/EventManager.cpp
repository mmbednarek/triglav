#include "EventManager.hpp"

namespace triglav {

u32 EventManager::register_callback(const Name event, void* object, void* func)
{
   std::lock_guard lk{m_mutex};

   const u32 result = m_top_callback++;
   m_callbacks[result] = {event, object, func};
   m_event_to_callback.insert({event, result});
   return result;
}

void EventManager::remove_callback(const u32 callback_id)
{
   std::lock_guard lk{m_mutex};

   const Name event_name = m_callbacks.at(callback_id).event;
   const auto [a, b] = m_event_to_callback.equal_range(event_name);
   const auto it = std::find_if(a, b, [callback_id](const std::pair<Name, u32>& p) { return p.second == callback_id; });
   m_event_to_callback.erase(it);
   m_callbacks.erase(callback_id);
}

EventManager& EventManager::the()
{
   static EventManager instance;
   return instance;
}

}// namespace triglav