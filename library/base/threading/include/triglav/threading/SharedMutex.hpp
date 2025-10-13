#pragma once

#include <memory>

namespace triglav::threading {

struct SharedMutexHandle;

class SharedMutex
{
 public:
   SharedMutex();
   ~SharedMutex();

   void lock_shared();
   bool try_lock_shared();
   void unlock_shared();

   void lock();
   bool try_lock();
   void unlock();

 private:
   std::unique_ptr<SharedMutexHandle> m_handle;
};

}// namespace triglav::threading