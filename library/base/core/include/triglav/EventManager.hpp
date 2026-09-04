#pragma once

#include "Name.hpp"

#include <map>
#include <mutex>
#include <span>
#include <vector>

namespace triglav {

class EventManager
{
 public:
   using CallbackRange = std::pair<std::multimap<Name, u32>::iterator, std::multimap<Name, u32>::iterator>;
   struct Callback
   {
      Name event;
      void* object;
      void* func;
   };

   u32 register_callback(Name event, void* object, void* func);
   void remove_callback(u32 callback_id);

   template<typename F>
   void iterate_event_callbacks(const Name event, F fn)
   {
      static constexpr ptrdiff_t SMALL_STORE_LIMIT = 6;

      std::vector<Callback> large_store{};
      std::array<Callback, SMALL_STORE_LIMIT> small_store{};
      std::span<Callback> callbacks_span;

      {
         std::lock_guard lk{m_mutex};

         const auto [a, b] = m_event_to_callback.equal_range(event);
         const auto count = std::distance(a, b);
         if (count <= SMALL_STORE_LIMIT) {
            std::transform(a, b, small_store.begin(), [&](const std::pair<Name, u32>& p) { return m_callbacks[p.second]; });
            callbacks_span = {small_store.begin(), small_store.begin() + count};
         } else {
            large_store.resize(count);
            std::transform(a, b, large_store.begin(), [&](const std::pair<Name, u32>& p) { return m_callbacks[p.second]; });
            callbacks_span = large_store;
         }
      }

      for (const auto& cb : callbacks_span) {
         fn(cb.object, cb.func);
      }
   }

   static EventManager& the();

 private:
   std::multimap<Name, u32> m_event_to_callback;
   std::map<u32, Callback> m_callbacks;
   std::mutex m_mutex;
   u32 m_top_callback = 0;
};

}// namespace triglav