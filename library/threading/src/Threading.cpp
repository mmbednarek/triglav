#include "Threading.h"

#include "ThreadPool.h"

namespace triglav::threading {

thread_local ThreadID g_threadID{};

void set_thread_id(const ThreadID id)
{
   g_threadID = id;
}

ThreadID this_thread_id()
{
   return g_threadID;
}

u32 total_thread_count()
{
   return 1 + ThreadPool::the().thread_count();
}

}// namespace triglav::threading
