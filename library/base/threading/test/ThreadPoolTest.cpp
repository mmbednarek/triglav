#include "triglav/testing_core/GTest.hpp"

#include "triglav/threading/ThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <iostream>

using triglav::threading::ThreadPool;

TEST(ThreadPool, CanInitializeAndQuit)
{
   ThreadPool pool;
   pool.initialize(1);
   pool.quit();
}

TEST(ThreadPool, CanProcessJobs)
{
   std::condition_variable cv{};
   std::mutex mtx{};

   int counter{0};
   auto job = [&] {
      std::unique_lock lk{mtx};
      ++counter;
      lk.unlock();
      cv.notify_one();
   };

   ThreadPool pool;
   pool.initialize(4);

   for (int i = 0; i < 100; ++i) {
      pool.issue_job(job);
   }

   std::unique_lock lk{mtx};
   cv.wait(lk, [&counter] { return counter == 100; });
   lk.unlock();

   pool.quit();
}

TEST(ThreadPool, CanIssueJobsInside)
{
   std::condition_variable cv{};
   std::mutex mtx{};
   ThreadPool pool;

   int counter{0};
   auto job = [&] {
      std::unique_lock lk{mtx};

      if (counter == 5) {
         pool.issue_job([&] {
            std::unique_lock lk{mtx};
            ++counter;
            lk.unlock();
            cv.notify_one();
         });
      } else {
         ++counter;
      }

      lk.unlock();
      cv.notify_one();
   };

   pool.initialize(4);

   for (int i = 0; i < 10; ++i) {
      pool.issue_job(job);
   }

   std::unique_lock lk{mtx};
   cv.wait(lk, [&counter] { return counter == 10; });
   lk.unlock();

   pool.quit();
}

TEST(ThreadPool, CanBeIdle)
{
   using namespace std::chrono_literals;

   std::condition_variable cv{};
   std::mutex mtx{};

   int counter{0};
   auto job = [&] {
      std::unique_lock lk{mtx};
      ++counter;
      lk.unlock();
      cv.notify_one();
   };

   ThreadPool pool;
   pool.initialize(4);

   for (int i = 0; i < 10; ++i) {
      pool.issue_job(job);
   }
   std::this_thread::sleep_for(20ms);
   for (int i = 0; i < 10; ++i) {
      pool.issue_job(job);
   }
   std::this_thread::sleep_for(20ms);
   for (int i = 0; i < 10; ++i) {
      pool.issue_job(job);
   }

   std::unique_lock lk{mtx};
   cv.wait(lk, [&counter] { return counter == 30; });
   lk.unlock();

   pool.quit();
}
