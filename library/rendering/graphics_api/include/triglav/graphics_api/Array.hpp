#pragma once

#include "Buffer.hpp"
#include "Device.hpp"

namespace triglav::graphics_api {

template<BufferUsageFlags CBufferUsage, typename TValue>
class Array
{
 public:
   Array(Device& device, const size_t element_count, const BufferUsageFlags additional_flags = 0) :
       m_device(device),
       m_buffer(
          GAPI_CHECK(device.create_buffer(CBufferUsage | BufferUsage::TransferDst | additional_flags, element_count * sizeof(TValue)))),
       m_element_count(element_count)
   {
   }

   Array(const Array& other) = delete;
   Array& operator=(const Array& other) = delete;

   Array(Array&& other) noexcept :
       m_device(other.m_device),
       m_buffer(std::move(other.m_buffer)),
       m_element_count(std::exchange(other.m_element_count, 0))
   {
   }

   Array& operator=(Array&& other) noexcept
   {
      if (this == &other)
         return *this;

      m_buffer = std::move(other.m_buffer);
      m_element_count = std::exchange(other.m_element_count, 0);

      return *this;
   }

   Array(Device& device, const std::vector<TValue>& values) :
       Array(device, values.size())
   {
      this->write(values.data(), values.size());
   }

   Status write(const TValue* source, const size_t count)
   {
      assert(count <= m_element_count);
      return m_buffer.write_indirect(source, count * sizeof(TValue));
   }

   [[nodiscard]] const Buffer& buffer() const
   {
      return m_buffer;
   }

   [[nodiscard]] Buffer& buffer()
   {
      return m_buffer;
   }

   [[nodiscard]] size_t count() const
   {
      return m_element_count;
   }

 private:
   Device& m_device;
   Buffer m_buffer;
   size_t m_element_count;
};

template<typename TVertex>
using VertexArray = Array<BufferUsage::VertexBuffer, TVertex>;

template<typename TObject>
using StorageArray = Array<BufferUsage::StorageBuffer, TObject>;

template<typename TObject>
using StagingArray = Array<BufferUsage::StorageBuffer | BufferUsage::HostVisible | BufferUsage::TransferSrc, TObject>;

using IndexArray = Array<BufferUsage::IndexBuffer, uint32_t>;

template<typename TVertex>
struct Mesh
{
   VertexArray<TVertex> vertices;
   IndexArray indices;
};

}// namespace triglav::graphics_api