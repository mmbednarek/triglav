#include "ThreadPool.hpp"

#include "SafeAccess.hpp"

#include <algorithm>

namespace triglav::threading {

void ThreadPool::initialize(const u32 count)
{
   m_threads.reserve(count);
   std::generate_n(std::back_inserter(m_threads), count,
                   [this, i = g_worker_thread_beg]() mutable { return std::thread(&ThreadPool::thread_entrypoint, this, i++); });
}

void ThreadPool::issue_job(Job&& job)
{
   m_queue.push(std::move(job));
   m_job_is_ready_cv.notify_one();
}

void ThreadPool::thread_routine()
{
   std::unique_lock lk{m_job_is_ready_mutex};
   m_job_is_ready_cv.wait(lk, [this] { return this->has_jobs_or_is_quitting(); });

   // ignore the job if quitting
   if (m_state.load() == State::Quitting)
      return;

   auto object = m_queue.pop();
   if (not object.has_value())
      return;

   lk.unlock();

   (*object)();
}

void ThreadPool::thread_entrypoint(const ThreadID thread_id)
{
   set_thread_id(thread_id);

   try {
      while (m_state.load() != State::Quitting) {
         this->thread_routine();
      }
   } catch (std::exception& e) {
      log_error("Exception occurred: {}, exiting...", e.what());
      flush_logs();
      std::exit(EXIT_FAILURE);
   } catch (...) {
      log_error("Unknown exception occurred, exiting...");
      flush_logs();
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
   m_job_is_ready_cv.notify_all();

   std::this_thread::sleep_for(std::chrono::milliseconds(32));

   for (auto& thread : m_threads) {
      m_job_is_ready_cv.notify_all();
      thread.join();
   }
   m_threads.clear();
}

ThreadPool& ThreadPool::the()
{
   static ThreadPool thread_pool;
   return thread_pool;
}

u32 ThreadPool::thread_count() const
{
   return static_cast<u32>(m_threads.size());
}

}// namespace triglav::threading