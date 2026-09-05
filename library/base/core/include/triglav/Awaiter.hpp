#include "Event.hpp"

#define TG_DEFINE_AWAITER(awaiter_name, producer, event, ...) \
   class awaiter_name                                         \
   {                                                          \
    public:                                                   \
      using Self = awaiter_name;                              \
      explicit awaiter_name(producer& in_producer) :          \
          TG_CONNECT(in_producer, event, callback_##event)    \
      {                                                       \
      }                                                       \
      void callback_##event(__VA_ARGS__)                      \
      {                                                       \
         {                                                    \
            std::lock_guard guard(m_mutex);                   \
            m_ready = true;                                   \
         }                                                    \
         m_cond.notify_one();                                 \
      }                                                       \
      void await()                                            \
      {                                                       \
         std::unique_lock lock(m_mutex);                      \
         m_cond.wait(lock, [this] { return m_ready; });       \
      }                                                       \
                                                              \
    private:                                                  \
      std::mutex m_mutex;                                     \
      std::condition_variable m_cond;                         \
      bool m_ready = false;                                   \
      TG_SINK(producer, event);                               \
   };
