#include "SharedMutex.hpp"

#include <cassert>
#include <pthread.h>

namespace triglav::threading {

struct SharedMutexHandle
{
   pthread_rwlock_t lock;
};

SharedMutex::SharedMutex() :
    m_handle(std::make_unique<SharedMutexHandle>())
{
   m_handle->lock = PTHREAD_RWLOCK_INITIALIZER;
}

SharedMutex::~SharedMutex()
{
   if (m_handle != nullptr) {
      pthread_rwlock_destroy(&m_handle->lock);
   }
}

void SharedMutex::lock_shared()
{
   int result{};
   do {
      result = pthread_rwlock_rdlock(&m_handle->lock);
   } while (result == EAGAIN);

   assert(result == 0);
}

bool SharedMutex::try_lock_shared()
{
   return pthread_rwlock_tryrdlock(&m_handle->lock) == 0;
}

void SharedMutex::unlock_shared()
{
   assert(pthread_rwlock_unlock(&m_handle->lock) == 0);
}

void SharedMutex::lock()
{
   assert(pthread_rwlock_wrlock(&m_handle->lock) == 0);
}

bool SharedMutex::try_lock()
{
   assert(pthread_rwlock_trywrlock(&m_handle->lock) == 0);
}

void SharedMutex::unlock()
{
   assert(pthread_rwlock_unlock(&m_handle->lock) == 0);
}

}// namespace triglav::threading