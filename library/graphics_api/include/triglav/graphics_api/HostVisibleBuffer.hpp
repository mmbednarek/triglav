#pragma once

#include "Device.hpp"

namespace triglav::graphics_api {

template<BufferUsage CAdditionalUsageFlags, typename TValue>
class HostVisibleBuffer
{
 public:
   explicit HostVisibleBuffer(Device& device) :
       m_buffer(GAPI_CHECK(device.create_buffer(BufferUsage::HostVisible | CAdditionalUsageFlags, sizeof(TValue)))),
       m_mappedMemory(GAPI_CHECK(m_buffer.map_memory()))
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

   [[nodiscard]] const Buffer& buffer() const
   {
      return m_buffer;
   }

 private:
   Buffer m_buffer;
   MappedMemory m_mappedMemory;
};

template<typename TValue>
using UniformBuffer = HostVisibleBuffer<BufferUsage::UniformBuffer, TValue>;

}// namespace triglav::graphics_api