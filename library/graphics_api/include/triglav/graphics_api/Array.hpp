#pragma once

#include "Buffer.h"
#include "Device.h"

namespace triglav::graphics_api {

template<BufferPurpose CBufferPurpose, typename TValue>
class Array
{
 public:
   Array(Device &device, const size_t element_count) :
       m_device(device),
       m_buffer(GAPI_CHECK(device.create_buffer(CBufferPurpose, element_count * sizeof(TValue)))),
       m_elementCount(element_count)
   {
   }

   Array(const Array &other)            = delete;
   Array &operator=(const Array &other) = delete;

   Array(Array &&other) noexcept :
       m_device(other.m_device),
       m_buffer(std::move(other.m_buffer)),
       m_elementCount(std::exchange(other.m_elementCount, 0))
   {
   }

   Array &operator=(Array &&other) noexcept
   {
      if (this == &other)
         return *this;

      m_buffer       = std::move(other.m_buffer);
      m_elementCount = std::exchange(other.m_elementCount, 0);

      return *this;
   }

   Array(Device &device, const std::vector<TValue> &values) :
       Array(device, values.size())
   {
      this->write(values.data(), values.size());
   }

   void write(const TValue *source, const size_t count)
   {
      assert(count <= m_elementCount);
      write_to_buffer(m_device, m_buffer, source, count * sizeof(TValue));
   }

   [[nodiscard]] const Buffer &buffer() const
   {
      return m_buffer;
   }

   [[nodiscard]] size_t count() const
   {
      return m_elementCount;
   }

 private:
   Device &m_device;
   Buffer m_buffer;
   size_t m_elementCount;
};

template<typename TVertex>
using VertexArray = Array<BufferPurpose::VertexBuffer, TVertex>;

using IndexArray = Array<BufferPurpose::IndexBuffer, uint32_t>;

template<typename TVertex>
struct Mesh
{
   VertexArray<TVertex> vertices;
   IndexArray indices;
};

}// namespace graphics_api