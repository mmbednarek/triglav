#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "triglav/Int.hpp"
#include "triglav/Logging.hpp"

#include "DoubleBufferQueue.hpp"
#include "Threading.hpp"

namespace triglav::threading {

class ThreadPool
{
   TG_DEFINE_LOG_CATEGORY(ThreadPool)
 public:
   using Job = std::function<void()>;

   enum class State
   {
      Uninitialized,
      Working,
      Quitting,
   };

   void initialize(u32 count);
   void issue_job(Job&& job);
   void thread_routine();
   void thread_entrypoint(ThreadID thread_id);
   void quit();
   [[nodiscard]] u32 thread_count() const;

   [[nodiscard]] static ThreadPool& the();

 private:
   [[nodiscard]] bool has_jobs_or_is_quitting();

   std::vector<std::thread> m_threads;
   std::atomic<State> m_state{State::Uninitialized};
   DoubleBufferQueue<Job> m_queue;
   std::mutex m_job_is_ready_mutex;
   std::condition_variable m_job_is_ready_cv;
};

}// namespace triglav::threading
