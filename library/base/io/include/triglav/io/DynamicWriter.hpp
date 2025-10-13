#pragma once

#include "Stream.hpp"

#include "triglav/Int.hpp"

#include <memory>

namespace triglav::io {

class DynamicWriter : public IWriter
{
 public:
   explicit DynamicWriter(MemorySize initialCapacity = 128);
   ~DynamicWriter() override;

   DynamicWriter(const DynamicWriter& other);
   DynamicWriter& operator=(const DynamicWriter& other);
   DynamicWriter(DynamicWriter&& other) noexcept;
   DynamicWriter& operator=(DynamicWriter&& other) noexcept;

   [[nodiscard]] Result<MemorySize> write(std::span<const u8> buffer) override;
   [[nodiscard]] Result<MemorySize> align(MemorySize alignment);

   [[nodiscard]] std::span<u8> span() const;
   [[nodiscard]] u8* data() const;
   [[nodiscard]] MemorySize size() const;
   [[nodiscard]] MemorySize capacity() const;

 private:
   void set_position(MemorySize newPosition);

   std::allocator<u8> m_allocator;

   u8* m_buffer;
   MemorySize m_currentCapacity{};
   MemorySize m_position{};
};

}// namespace triglav::io
