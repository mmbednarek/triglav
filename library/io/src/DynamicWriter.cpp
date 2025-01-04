#include "DynamicWriter.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace triglav::io {

DynamicWriter::DynamicWriter(MemorySize initialCapacity) :
    m_buffer(m_allocator.allocate(initialCapacity)),
    m_currentCapacity(initialCapacity),
    m_position(0)
{
}

DynamicWriter::~DynamicWriter()
{
   m_allocator.deallocate(m_buffer, m_currentCapacity);
}

DynamicWriter::DynamicWriter(const DynamicWriter& other) :
    m_buffer(m_allocator.allocate(other.m_currentCapacity)),
    m_currentCapacity(other.m_currentCapacity),
    m_position(other.m_position)
{
   std::memcpy(m_buffer, other.m_buffer, m_position);
}

DynamicWriter& DynamicWriter::operator=(const DynamicWriter& other)
{
   if (this == &other) {
      return *this;
   }

   m_allocator.deallocate(m_buffer, m_currentCapacity);
   m_buffer = m_allocator.allocate(other.m_currentCapacity);
   m_currentCapacity = other.m_currentCapacity;
   m_position = other.m_position;
   return *this;
}

DynamicWriter::DynamicWriter(DynamicWriter&& other) noexcept :
    m_buffer(std::exchange(other.m_buffer, nullptr)),
    m_currentCapacity(std::exchange(other.m_currentCapacity, 0)),
    m_position(std::exchange(other.m_position, 0))
{
}

DynamicWriter& DynamicWriter::operator=(DynamicWriter&& other) noexcept
{
   m_buffer = std::exchange(other.m_buffer, nullptr);
   m_currentCapacity = std::exchange(other.m_currentCapacity, 0);
   m_position = std::exchange(other.m_position, 0);

   return *this;
}

Result<MemorySize> DynamicWriter::write(const std::span<const u8> buffer)
{
   const auto prevPosition = m_position;
   this->set_position(this->m_position + buffer.size());
   std::memcpy(m_buffer + prevPosition, buffer.data(), buffer.size());
   return buffer.size();
}

Result<MemorySize> DynamicWriter::align(const MemorySize alignment)
{
   const auto mod = m_position % alignment;
   const auto offset = mod == 0 ? 0 : alignment - mod;
   this->set_position(this->m_position + offset);
   return offset;
}

std::span<u8> DynamicWriter::span() const
{
   return std::span{m_buffer, m_position};
}

u8* DynamicWriter::data() const
{
   return m_buffer;
}

MemorySize DynamicWriter::size() const
{
   return m_position;
}

MemorySize DynamicWriter::capacity() const
{
   return m_currentCapacity;
}

void DynamicWriter::set_position(const MemorySize newPosition)
{
   if (newPosition > m_currentCapacity) {
      auto new_cap = std::max<MemorySize>(2 * m_currentCapacity, 128);
      while (newPosition > new_cap) {
         new_cap *= 2;
      }
      auto* new_buff = m_allocator.allocate(new_cap);

      std::memcpy(new_buff, m_buffer, m_position);

      m_allocator.deallocate(m_buffer, m_currentCapacity);
      m_buffer = new_buff;
      m_currentCapacity = new_cap;
   }

   m_position = newPosition;
}

}// namespace triglav::io
