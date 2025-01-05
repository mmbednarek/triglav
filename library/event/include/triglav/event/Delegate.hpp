#pragma once

#include "Entt.hpp"

#include <condition_variable>
#include <mutex>
#include <optional>

namespace triglav::event {

template<typename... TArgs>
class Delegate
{
 public:
   using Self = Delegate;
   using Sigh = entt::sigh<void(TArgs...)>;

   template<typename THandler>
   class Sink
   {
      friend Self;

    public:
      Sink(Self& delegate, THandler* handler) :
          m_sink{delegate.m_sigh},
          m_handler{handler}
      {
      }

      ~Sink()
      {
         if (m_handler != nullptr)
            disconnect();
      }

      void disconnect()
      {
         m_sink.disconnect(m_handler);
         m_handler = nullptr;
      }

    private:
      entt::sink<Sigh> m_sink;
      THandler* m_handler{};
   };

   template<typename THandler>
   using OptSink = std::optional<Sink<THandler>>;

   template<typename... TCallArgs>
   void publish(TCallArgs&&... args) const
   {
      std::unique_lock lk{m_mutex};
      m_sigh.publish(std::forward<TCallArgs>(args)...);
   }

   template<auto CHandleFunction, typename THandler>
   Sink<THandler> connect(THandler* handler)
   {
      std::unique_lock lk{m_mutex};

      Sink<THandler> out(*this, handler);
      out.m_sink.template connect<CHandleFunction>(handler);
      return out;
   }

   template<auto CHandleFunction, typename THandler>
   void connect_to(OptSink<THandler>& sink, THandler* handler)
   {
      sink.emplace(this->template connect<CHandleFunction>(handler));
   }

   mutable std::mutex m_mutex;
   Sigh m_sigh;
};

}// namespace triglav::event

#define TG_EVENT(name, ...)                                        \
   using name##Delegate = ::triglav::event::Delegate<__VA_ARGS__>; \
   name##Delegate event_##name;

#define TG_SINK(sender, name) sender::name##Delegate::Sink<Self> sink_##name

#define TG_CONNECT(obj, name, func) sink_##name(obj.event_##name.connect<&Self::func>(this))

#define TG_DEFINE_AWAITER(awaiter_name, producer, event)  \
   class awaiter_name                                     \
   {                                                      \
    public:                                               \
      using Self = awaiter_name;                          \
      explicit awaiter_name(producer& inProducer) :       \
          TG_CONNECT(inProducer, event, callback_##event) \
      {                                                   \
      }                                                   \
      void callback_##event()                             \
      {                                                   \
         {                                                \
            std::lock_guard guard(m_mutex);               \
            m_ready = true;                               \
         }                                                \
         m_cond.notify_one();                             \
      }                                                   \
      void await()                                        \
      {                                                   \
         std::unique_lock lock(m_mutex);                  \
         m_cond.wait(lock, [this] { return m_ready; });   \
      }                                                   \
                                                          \
    private:                                              \
      std::mutex m_mutex;                                 \
      std::condition_variable m_cond;                     \
      bool m_ready = false;                               \
      TG_SINK(producer, event);                           \
   };
