#pragma once

#include "Entt.hpp"

#include <mutex>
#include <optional>
#include <shared_mutex>

namespace triglav {

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
   void publish(TCallArgs&&... args)
   {
      std::shared_lock lk{m_mutex};
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

   std::shared_mutex m_mutex;
   Sigh m_sigh;
};

}// namespace triglav