#pragma once

#include "EventManager.hpp"
#include "Macros.hpp"
#include "Name.hpp"

namespace triglav {

constexpr u32 NULL_CALLBACK_ID = ~0u;
constexpr u64 EVENT_OBJECT_MULT = 4442237798131759ull;
constexpr u64 EVENT_NAME_MULT = 6773286701986361ull;

class Sink
{
 public:
   Sink();
   explicit Sink(u32 callback_id);
   ~Sink();

   Sink(Sink&& other) noexcept;
   Sink& operator=(Sink&& other) noexcept;

   TG_DELETE_COPY(Sink)

   u32 release();

 private:
   u32 m_callback_id{};
};

template<typename... TArgs>
class Event
{
 public:
   Event() :
       m_event_id(EventManager::the().allocate_event_id())
   {
   }

   void publish(TArgs... args) const
   {
      EventManager::the().iterate_event_callbacks(m_event_id, [&](void* object, void* callback) {
         const auto fn_callback = reinterpret_cast<void (*)(void*, TArgs...)>(callback);
         fn_callback(object, std::forward<TArgs>(args)...);
      });
   }

   template<auto CFunc, typename T>
   [[nodiscard]] Sink connect(T& object) const
   {
      return Sink(EventManager::the().register_callback(m_event_id, &object, reinterpret_cast<void*>(+[](void* handle, TArgs... args) {
                                                           (static_cast<T*>(handle)->*CFunc)(std::forward<TArgs>(args)...);
                                                        })));
   }

   [[nodiscard]] EventID event_id() const
   {
      return m_event_id;
   }

 private:
   EventID m_event_id;
};

}// namespace triglav

#define TG_EVENT(event_name, ...) ::triglav::Event<__VA_ARGS__> TG_CONCAT(event_, event_name){};

#define TG_SINK(sender, name) ::triglav::Sink TG_CONCAT(sink_, name)
#define TG_OPT_SINK(sender, name) TG_SINK(sender, name)
#define TG_OPT_NAMED_SINK(sender, name, sink_name) ::triglav::Sink TG_CONCAT(sink_, sink_name)

#define TG_CONNECT_NAMED_OPT(obj, name, sink_name, func) \
   TG_CONCAT(sink_, sink_name) = ((obj).TG_CONCAT(event_, name).connect<&Self::func>(*this))
#define TG_CONNECT(obj, name, func) TG_CONCAT(sink_, name)((obj).TG_CONCAT(event_, name).connect<&Self::func>(*this))
#define TG_CONNECT_OPT(obj, name, func) TG_CONNECT_NAMED_OPT(obj, name, name, func)
