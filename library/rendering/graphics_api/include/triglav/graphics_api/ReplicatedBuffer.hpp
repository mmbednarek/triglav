#pragma once

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"

#include <mutex>

namespace triglav::graphics_api {

template<typename TValue>
class BufferLock
{
 public:
   BufferLock(Buffer& locked_buffer, std::mutex& mutex) :
       m_mapped_memory(GAPI_CHECK(locked_buffer.map_memory())),
       m_lock(mutex)
   {
   }

   [[nodiscard]] TValue& operator*() const
   {
      return *static_cast<TValue*>(*m_mapped_memory);
   }

   [[nodiscard]] TValue* operator->() const
   {
      return static_cast<TValue*>(*m_mapped_memory);
   }

 private:
   MappedMemory m_mapped_memory;
   std::unique_lock<std::mutex> m_lock;
};

template<BufferUsage CAdditionalUsageFlags, typename TValue>
class ReplicatedBuffer
{
 public:
   explicit ReplicatedBuffer(Device& device) :
       m_host_buffer(
          GAPI_CHECK(device.create_buffer(BufferUsage::TransferSrc | BufferUsage::HostVisible | CAdditionalUsageFlags, sizeof(TValue)))),
       m_device_buffer(GAPI_CHECK(device.create_buffer(BufferUsage::TransferDst | CAdditionalUsageFlags, sizeof(TValue))))
   {
   }

   BufferLock<TValue> lock()
   {
      return BufferLock<TValue>{m_host_buffer, m_mutex};
   }

   void sync(CommandList& cmd_list)
   {
      cmd_list.copy_buffer(m_host_buffer, m_device_buffer);
   }

   [[nodiscard]] const Buffer& buffer() const
   {
      return m_device_buffer;
   }

 private:
   Buffer m_host_buffer;
   Buffer m_device_buffer;
   std::mutex m_mutex;
};

template<typename TValue>
using UniformReplicatedBuffer = ReplicatedBuffer<BufferUsage::UniformBuffer, TValue>;

}// namespace triglav::graphics_api
