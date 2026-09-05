#pragma once

#include "Int.hpp"

#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <span>
#include <vector>

namespace triglav {

using EventID = u32;
using CallbackID = u32;

class EventManager
{
 public:
   struct Callback
   {
      EventID event_id;
      void* object;
      void* func;
   };

   CallbackID register_callback(EventID event, void* object, void* func);
   void remove_callback(CallbackID callback_id);
   EventID allocate_event_id();

   template<typename F>
   void iterate_event_callbacks(const EventID event, F fn)
   {
      static constexpr std::ptrdiff_t SMALL_STORE_LIMIT = 6;

      std::vector<Callback> large_store{};
      std::array<Callback, SMALL_STORE_LIMIT> small_store{};
      std::span<Callback> callbacks_span;

      {
         std::lock_guard lk{m_mutex};

         const auto [a, b] = m_event_to_callback.equal_range(event);
         const auto count = std::distance(a, b);
         if (count <= SMALL_STORE_LIMIT) {
            std::transform(a, b, small_store.begin(), [&](const std::pair<EventID, CallbackID>& p) { return m_callbacks[p.second]; });
            callbacks_span = {small_store.begin(), small_store.begin() + count};
         } else {
            large_store.resize(count);
            std::transform(a, b, large_store.begin(), [&](const std::pair<EventID, CallbackID>& p) { return m_callbacks[p.second]; });
            callbacks_span = large_store;
         }
      }

      for (const auto& cb : callbacks_span) {
         fn(cb.object, cb.func);
      }
   }

   static EventManager& the();

 private:
   std::multimap<EventID, CallbackID> m_event_to_callback;
   std::map<CallbackID, Callback> m_callbacks;
   std::mutex m_mutex;
   std::atomic_uint32_t m_top_callback = 0;
   std::atomic_uint32_t m_top_event_id = 0;
};

}// namespace triglav