#include "SharedMutex.hpp"

// #include <synchapi.h>
#include <windows.h>

namespace triglav::threading {

struct SharedMutexHandle
{
   SRWLOCK lock;
};

SharedMutex::SharedMutex() :
    m_handle(std::make_unique<SharedMutexHandle>())
{
   InitializeSRWLock(&m_handle->lock);
}

SharedMutex::~SharedMutex() = default;

void SharedMutex::lock_shared()
{
   AcquireSRWLockShared(&m_handle->lock);
}

bool SharedMutex::try_lock_shared()
{
   return TryAcquireSRWLockShared(&m_handle->lock);
}

void SharedMutex::unlock_shared()
{
   ReleaseSRWLockShared(&m_handle->lock);
}

void SharedMutex::lock()
{
   AcquireSRWLockExclusive(&m_handle->lock);
}

bool SharedMutex::try_lock()
{
   return TryAcquireSRWLockExclusive(&m_handle->lock);
}

void SharedMutex::unlock()
{
   ReleaseSRWLockExclusive(&m_handle->lock);
}

}// namespace triglav::threading
