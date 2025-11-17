#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace triglav::threading {

template<typename TObject>
class DoubleBufferQueue
{
 public:
   using Queue = std::queue<TObject>;

   DoubleBufferQueue() :
       m_push_queue{&m_first},
       m_pop_queue{&m_second}
   {
   }

   void push(TObject&& object)
   {
      std::unique_lock lk{m_push_mutex};
      m_push_queue->push(std::move(object));
   }

   [[nodiscard]] std::optional<TObject> pop()
   {
      if (m_pop_queue->empty())
         return std::nullopt;

      auto object = std::move(m_pop_queue->front());
      m_pop_queue->pop();
      return std::optional{std::move(object)};
   }

   [[nodiscard]] bool is_empty()
   {
      return m_pop_queue->empty();
   }

   void swap()
   {
      std::unique_lock lk{m_push_mutex};
      std::swap(m_push_queue, m_pop_queue);
   }

 private:
   Queue m_first;
   Queue m_second;
   std::mutex m_push_mutex;
   Queue* m_push_queue;
   Queue* m_pop_queue;
};

}// namespace triglav::threading