#pragma once

#include "SharedMutex.hpp"

#include <mutex>
#include <shared_mutex>
#include <utility>

namespace triglav::threading {

template<typename TObject, typename TLock = std::unique_lock<std::mutex>>
class SafeAccessor
{
 public:
   using Mutex = typename TLock::mutex_type;
   using ObjectRef = std::conditional_t<std::is_const_v<TObject>, const TObject&, TObject&>;

   SafeAccessor(ObjectRef object, Mutex& mutex) :
       m_object(object),
       m_lock(mutex)
   {
   }

   ObjectRef operator*()
   {
      return m_object;
   }

   TObject* operator->()
   {
      return &m_object;
   }

   ObjectRef value()
   {
      return m_object;
   }

   const ObjectRef value() const
   {
      return m_object;
   }

 private:
   ObjectRef m_object;
   TLock m_lock;
};

template<typename TObject, typename TMutex = std::mutex>
class SafeAccess
{
 public:
   template<typename... TArgs>
   explicit SafeAccess(TArgs&&... args) :
       m_object(std::forward<TArgs>(args)...)
   {
   }

   SafeAccess& operator=(TObject&& obj) noexcept
   {
      std::unique_lock lk(m_mutex);
      m_object = std::forward<TObject>(obj);
      return *this;
   }

   SafeAccessor<TObject, std::unique_lock<TMutex>> access()
   {
      return SafeAccessor<TObject, std::unique_lock<TMutex>>{m_object, m_mutex};
   }

   SafeAccessor<const TObject, std::unique_lock<TMutex>> const_access() const
   {
      return SafeAccessor<const TObject, std::unique_lock<TMutex>>{m_object, m_mutex};
   }

 protected:
   TObject m_object;
   mutable TMutex m_mutex;
};

template<typename TObject>
class SafeReadWriteAccess : public SafeAccess<TObject, SharedMutex>
{
 public:
   using Parent = SafeAccess<TObject, SharedMutex>;

   template<typename... TArgs>
   explicit SafeReadWriteAccess(TArgs&&... args) :
       Parent(std::forward<TArgs>(args)...)
   {
   }

   SafeAccessor<const TObject, std::shared_lock<SharedMutex>> read_access() const
   {
      return SafeAccessor<const TObject, std::shared_lock<SharedMutex>>{Parent::m_object, Parent::m_mutex};
   }
};

}// namespace triglav::threading