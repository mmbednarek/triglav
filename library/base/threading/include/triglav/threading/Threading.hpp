#pragma once

#include "triglav/Int.hpp"

namespace triglav::threading {

using ThreadID = u32;

constexpr ThreadID g_main_thread = 0;
constexpr ThreadID g_worker_thread_beg = 1;

void set_thread_id(ThreadID id);
ThreadID this_thread_id();
u32 total_thread_count();

}// namespace triglav::threading