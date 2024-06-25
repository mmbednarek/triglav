#pragma once

#include "Buffer.h"
#include "CommandList.h"
#include "Device.h"

#include <mutex>

namespace triglav::graphics_api {

template<typename TValue>
class BufferLock
{
 public:
   BufferLock(Buffer& lockedBuffer, std::mutex& mutex) :
       m_mappedMemory(GAPI_CHECK(lockedBuffer.map_memory())),
       m_lock(mutex)
   {
   }

   [[nodiscard]] TValue& operator*() const
   {
      return *static_cast<TValue*>(*m_mappedMemory);
   }

   [[nodiscard]] TValue* operator->() const
   {
      return static_cast<TValue*>(*m_mappedMemory);
   }

 private:
   MappedMemory m_mappedMemory;
   std::unique_lock<std::mutex> m_lock;
};

template<BufferUsage CAdditionalUsageFlags, typename TValue>
class ReplicatedBuffer
{
 public:
   explicit ReplicatedBuffer(Device& device) :
       m_hostBuffer(
          GAPI_CHECK(device.create_buffer(BufferUsage::TransferSrc | BufferUsage::HostVisible | CAdditionalUsageFlags, sizeof(TValue)))),
       m_deviceBuffer(GAPI_CHECK(device.create_buffer(BufferUsage::TransferDst | CAdditionalUsageFlags, sizeof(TValue))))
   {
   }

   BufferLock<TValue> lock()
   {
      return BufferLock<TValue>{m_hostBuffer, m_mutex};
   }

   void sync(CommandList& cmdList)
   {
      cmdList.copy_buffer(m_hostBuffer, m_deviceBuffer);
   }

   [[nodiscard]] const Buffer& buffer() const
   {
      return m_deviceBuffer;
   }

 private:
   Buffer m_hostBuffer;
   Buffer m_deviceBuffer;
   std::mutex m_mutex;
};

template<typename TValue>
using UniformReplicatedBuffer = ReplicatedBuffer<BufferUsage::UniformBuffer, TValue>;

}// namespace triglav::graphics_api
