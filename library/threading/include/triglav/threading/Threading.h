#pragma once

#include "triglav/Int.hpp"

namespace triglav::threading {

using ThreadID = u32;

constexpr ThreadID g_mainThread = 0;
constexpr ThreadID g_workerThreadBeg = 0;

void set_thread_id(ThreadID id);
ThreadID this_thread_id();
u32 total_thread_count();

}// namespace triglav::threading