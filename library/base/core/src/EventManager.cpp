#include "EventManager.hpp"

#include <cassert>

namespace triglav {

CallbackID EventManager::register_callback(const EventID event, void* object, void* func)
{
   std::lock_guard lk{m_mutex};

   const CallbackID result = m_top_callback++;
   assert(!m_callbacks.contains(result));
   m_callbacks[result] = {event, object, func};
   m_event_to_callback.insert({event, result});
   return result;
}

void EventManager::remove_callback(const CallbackID callback_id)
{
   std::lock_guard lk{m_mutex};

   const EventID event_id = m_callbacks.at(callback_id).event_id;
   const auto [a, b] = m_event_to_callback.equal_range(event_id);
   const auto it = std::find_if(a, b, [callback_id](const std::pair<EventID, CallbackID>& p) { return p.second == callback_id; });
   m_event_to_callback.erase(it);
   m_callbacks.erase(callback_id);
}

EventID EventManager::allocate_event_id()
{
   return m_top_event_id++;
}

EventManager& EventManager::the()
{
   static EventManager instance;
   return instance;
}

}// namespace triglav