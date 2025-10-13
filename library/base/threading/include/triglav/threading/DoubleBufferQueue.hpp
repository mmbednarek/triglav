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
       m_pushQueue{&m_first},
       m_popQueue{&m_second}
   {
   }

   void push(TObject&& object)
   {
      std::unique_lock lk{m_pushMutex};
      m_pushQueue->push(std::move(object));
   }

   [[nodiscard]] std::optional<TObject> pop()
   {
      if (m_popQueue->empty())
         return std::nullopt;

      auto object = std::move(m_popQueue->front());
      m_popQueue->pop();
      return std::optional{std::move(object)};
   }

   [[nodiscard]] bool is_empty()
   {
      return m_popQueue->empty();
   }

   void swap()
   {
      std::unique_lock lk{m_pushMutex};
      std::swap(m_pushQueue, m_popQueue);
   }

 private:
   Queue m_first;
   Queue m_second;
   std::mutex m_pushMutex;
   Queue* m_pushQueue;
   Queue* m_popQueue;
};

}// namespace triglav::threading