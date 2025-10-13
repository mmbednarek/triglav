#include "HeapAllocator.hpp"

#include <algorithm>
#include <ranges>

namespace triglav::memory {

HeapAllocator::HeapAllocator(const SizeType size)
{
   m_freeList.emplace(0, size);
}

std::optional<MemorySize> HeapAllocator::allocate(const MemorySize size)
{
   // TODO: Find something better than linear search
   const auto it = std::ranges::find_if(m_freeList, [size](const std::pair<OffsetType, SizeType>& item) { return item.second >= size; });

   if (it->second == size) {
      const auto offset = it->first;
      m_freeList.erase(it);
      return offset;
   }

   it->second -= size;
   return it->first + it->second;
}

void HeapAllocator::free(const Area area)
{
   MemorySize nextItemSize = 0;

   bool shouldNextBeErased = false;
   const auto next = m_freeList.lower_bound(area.offset);
   if (next != m_freeList.end() && area.offset + area.size == next->first) {
      nextItemSize = next->second;
      shouldNextBeErased = true;
   }

   if (next == m_freeList.begin()) {
      if (shouldNextBeErased) {
         m_freeList.erase(next);
      }
      m_freeList.emplace(area.offset, area.size + nextItemSize);
      return;
   }

   const auto prev = std::prev(next);
   if (prev != m_freeList.end() && prev->first + prev->second == area.offset) {
      prev->second += area.size + nextItemSize;
   } else {
      m_freeList.emplace(area.offset, area.size + nextItemSize);
   }

   if (shouldNextBeErased) {
      m_freeList.erase(next);
   }
}

}// namespace triglav::memory
