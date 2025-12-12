#pragma once

#include "Memory.hpp"

#include "triglav/Int.hpp"

#include <map>
#include <optional>

namespace triglav::memory {

class HeapAllocator
{
 public:
   using OffsetType = MemorySize;
   using SizeType = MemorySize;

   explicit HeapAllocator(SizeType size);

   // Returns offset
   std::optional<MemorySize> allocate(MemorySize size);
   void free(Area area);

#if TG_HEAP_ALLOCATOR_TEST
   std::map<OffsetType, SizeType>& free_list()
   {
      return m_free_list;
   }
#endif// TG_HEAP_ALLOCATOR_TEST

 private:
   std::map<OffsetType, SizeType> m_free_list;
};

}// namespace triglav::memory
