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
   explicit Event(const Name object_name, const Name event_name) :
       m_event_name(EVENT_OBJECT_MULT * object_name + EVENT_NAME_MULT * event_name)
   {
   }

   explicit Event(const Name event_name) :
       m_event_name(event_name)
   {
   }

   void publish(TArgs... args)
   {
      EventManager::the().iterate_event_callbacks(m_event_name, [&](void* object, void* callback) {
         const auto fn_callback = reinterpret_cast<void (*)(void*, TArgs...)>(callback);
         fn_callback(object, std::forward<TArgs>(args)...);
      });
   }

   template<auto CFunc, typename T>
   [[nodiscard]] Sink connect(T& object) const
   {
      return Sink(EventManager::the().register_callback(m_event_name, &object, reinterpret_cast<void*>(+[](void* handle, TArgs... args) {
                                                           (static_cast<T*>(handle)->*CFunc)(std::forward<TArgs>(args)...);
                                                        })));
   }

 private:
   Name m_event_name;
};

}// namespace triglav

#define TG_EVENT_NEW(event_name, ...)                          \
   ::triglav::Event<__VA_ARGS__> TG_CONCAT(event_, event_name) \
   {                                                           \
      TAG, ::triglav::make_name_id(TG_STRING(event_name))      \
   }

#define TG_SINK_NEW(sender, name) ::triglav::Sink TG_CONCAT(sink_, name)
#define TG_OPT_SINK_NEW(sender, name) TG_SINK_NEW(sender, name)
#define TG_OPT_NAMED_SINK_NEW(sender, name, sink_name) ::triglav::Sink TG_CONCAT(sink_, sink_name)

#define TG_CONNECT_NAMED_OPT_NEW(obj, name, sink_name, func) \
   TG_CONCAT(sink_, sink_name) = ((obj).TG_CONCAT(event_, name).connect<&Self::func>(*this))
#define TG_CONNECT_NEW(obj, name, func) TG_CONCAT(sink_, name)((obj).TG_CONCAT(event_, name).connect<&Self::func>(*this))
#define TG_CONNECT_OPT_NEW(obj, name, func) TG_CONNECT_NAMED_OPT_NEW(obj, name, name, func)
