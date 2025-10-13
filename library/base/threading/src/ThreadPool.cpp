#include "ThreadPool.hpp"

#include "SafeAccess.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace triglav::threading {

void ThreadPool::initialize(const u32 count)
{
   m_threads.reserve(count);
   std::generate_n(std::back_inserter(m_threads), count,
                   [this, i = g_workerThreadBeg]() mutable { return std::thread(&ThreadPool::thread_entrypoint, this, i++); });
}

void ThreadPool::issue_job(Job&& job)
{
   m_queue.push(std::move(job));
   m_jobIsReadyCV.notify_one();
}

void ThreadPool::thread_routine()
{
   std::unique_lock lk{m_jobIsReadyMutex};
   m_jobIsReadyCV.wait(lk, [this] { return this->has_jobs_or_is_quitting(); });

   // ignore the job if quitting
   if (m_state.load() == State::Quitting)
      return;

   auto object = m_queue.pop();
   if (not object.has_value())
      return;

   lk.unlock();

   (*object)();
}

void ThreadPool::thread_entrypoint(const ThreadID threadId)
{
   set_thread_id(threadId);

   try {
      while (m_state.load() != State::Quitting) {
         this->thread_routine();
      }
   } catch (std::exception& e) {
      spdlog::error("thread-pool: exception occurred: {}, exiting...", e.what());
      std::exit(EXIT_FAILURE);
   } catch (...) {
      spdlog::error("thread-pool: unknown exception occurred, exiting...");
      std::exit(EXIT_FAILURE);
   }
}

bool ThreadPool::has_jobs_or_is_quitting()
{
   if (m_state.load() == State::Quitting)
      return true;

   if (not m_queue.is_empty())
      return true;

   m_queue.swap();

   return not m_queue.is_empty();
}

void ThreadPool::quit()
{
   m_state.store(State::Quitting);
   m_jobIsReadyCV.notify_all();

   std::this_thread::sleep_for(std::chrono::milliseconds(32));

   for (auto& thread : m_threads) {
      m_jobIsReadyCV.notify_all();
      thread.join();
   }
   m_threads.clear();
}

ThreadPool& ThreadPool::the()
{
   static ThreadPool threadPool;
   return threadPool;
}

u32 ThreadPool::thread_count() const
{
   return static_cast<u32>(m_threads.size());
}

}// namespace triglav::threading