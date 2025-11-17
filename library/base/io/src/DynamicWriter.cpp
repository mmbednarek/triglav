#include "DynamicWriter.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace triglav::io {

DynamicWriter::DynamicWriter(MemorySize initial_capacity) :
    m_buffer(m_allocator.allocate(initial_capacity)),
    m_current_capacity(initial_capacity),
    m_position(0)
{
}

DynamicWriter::~DynamicWriter()
{
   m_allocator.deallocate(m_buffer, m_current_capacity);
}

DynamicWriter::DynamicWriter(const DynamicWriter& other) :
    m_buffer(m_allocator.allocate(other.m_current_capacity)),
    m_current_capacity(other.m_current_capacity),
    m_position(other.m_position)
{
   std::memcpy(m_buffer, other.m_buffer, m_position);
}

DynamicWriter& DynamicWriter::operator=(const DynamicWriter& other)
{
   if (this == &other) {
      return *this;
   }

   m_allocator.deallocate(m_buffer, m_current_capacity);
   m_buffer = m_allocator.allocate(other.m_current_capacity);
   m_current_capacity = other.m_current_capacity;
   m_position = other.m_position;
   return *this;
}

DynamicWriter::DynamicWriter(DynamicWriter&& other) noexcept :
    m_buffer(std::exchange(other.m_buffer, nullptr)),
    m_current_capacity(std::exchange(other.m_current_capacity, 0)),
    m_position(std::exchange(other.m_position, 0))
{
}

DynamicWriter& DynamicWriter::operator=(DynamicWriter&& other) noexcept
{
   m_buffer = std::exchange(other.m_buffer, nullptr);
   m_current_capacity = std::exchange(other.m_current_capacity, 0);
   m_position = std::exchange(other.m_position, 0);

   return *this;
}

Result<MemorySize> DynamicWriter::write(const std::span<const u8> buffer)
{
   const auto prev_position = m_position;
   this->set_position(this->m_position + buffer.size());
   std::memcpy(m_buffer + prev_position, buffer.data(), buffer.size());
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
   return m_current_capacity;
}

void DynamicWriter::set_position(const MemorySize new_position)
{
   if (new_position > m_current_capacity) {
      auto new_cap = std::max<MemorySize>(2 * m_current_capacity, 128);
      while (new_position > new_cap) {
         new_cap *= 2;
      }
      auto* new_buff = m_allocator.allocate(new_cap);

      std::memcpy(new_buff, m_buffer, m_position);

      m_allocator.deallocate(m_buffer, m_current_capacity);
      m_buffer = new_buff;
      m_current_capacity = new_cap;
   }

   m_position = new_position;
}

}// namespace triglav::io
