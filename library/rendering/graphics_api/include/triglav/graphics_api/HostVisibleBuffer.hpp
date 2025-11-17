#pragma once

#include "Device.hpp"

namespace triglav::graphics_api {

template<BufferUsage CAdditionalUsageFlags, typename TValue>
class HostVisibleBuffer
{
 public:
   explicit HostVisibleBuffer(Device& device, const BufferUsage runtime_flags = BufferUsage::None) :
       m_buffer(GAPI_CHECK(device.create_buffer(BufferUsage::HostVisible | CAdditionalUsageFlags | runtime_flags, sizeof(TValue)))),
       m_mapped_memory(GAPI_CHECK(m_buffer.map_memory()))
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

   [[nodiscard]] const Buffer& buffer() const
   {
      return m_buffer;
   }

 private:
   Buffer m_buffer;
   MappedMemory m_mapped_memory;
};

template<typename TValue>
using UniformBuffer = HostVisibleBuffer<BufferUsage::UniformBuffer, TValue>;

}// namespace triglav::graphics_api