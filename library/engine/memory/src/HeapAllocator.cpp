#include "HeapAllocator.hpp"

#include <algorithm>
#include <ranges>

namespace triglav::memory {

HeapAllocator::HeapAllocator(const SizeType size)
{
   m_free_list.emplace(0, size);
}

std::optional<MemorySize> HeapAllocator::allocate(const MemorySize size)
{
   // TODO: Find something better than linear search
   const auto it = std::ranges::find_if(m_free_list, [size](const std::pair<OffsetType, SizeType>& item) { return item.second >= size; });
   if (it == m_free_list.end()) {
      return std::nullopt;
   }

   if (it->second == size) {
      const auto offset = it->first;
      m_free_list.erase(it);
      return offset;
   }

   it->second -= size;
   return it->first + it->second;
}

void HeapAllocator::free(const Area area)
{
   MemorySize next_item_size = 0;

   bool should_next_be_erased = false;
   const auto next = m_free_list.lower_bound(area.offset);
   if (next != m_free_list.end() && area.offset + area.size == next->first) {
      next_item_size = next->second;
      should_next_be_erased = true;
   }

   if (next == m_free_list.begin()) {
      if (should_next_be_erased) {
         m_free_list.erase(next);
      }
      m_free_list.emplace(area.offset, area.size + next_item_size);
      return;
   }

   const auto prev = std::prev(next);
   if (prev != m_free_list.end() && prev->first + prev->second == area.offset) {
      prev->second += area.size + next_item_size;
   } else {
      m_free_list.emplace(area.offset, area.size + next_item_size);
   }

   if (should_next_be_erased) {
      m_free_list.erase(next);
   }
}

}// namespace triglav::memory
