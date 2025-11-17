#include "Threading.hpp"

#include "ThreadPool.hpp"

namespace triglav::threading {

thread_local ThreadID g_thread_id{};

void set_thread_id(const ThreadID id)
{
   g_thread_id = id;
}

ThreadID this_thread_id()
{
   return g_thread_id;
}

u32 total_thread_count()
{
   return 1 + ThreadPool::the().thread_count();
}

}// namespace triglav::threading
