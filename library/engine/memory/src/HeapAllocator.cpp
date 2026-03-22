#include "HeapAllocator.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace triglav::memory {

HeapAllocator::HeapAllocator(const SizeType size) :
    m_size(size)
{
   m_free_list.emplace(0, size);
}

static constexpr MemorySize last_alignment(const MemorySize offset, const MemorySize alignment)
{
   return offset - (offset % alignment);
}

std::optional<MemorySize> HeapAllocator::allocate(const MemorySize size, const MemorySize alignment)
{
   if (size % alignment != 0) {
      return std::nullopt;
   }

   // TODO: Find something better than linear search
   const auto it = std::ranges::find_if(m_free_list, [size, alignment](const std::pair<OffsetType, SizeType>& item) {
      const auto aligned_offset = last_alignment(item.first + item.second, alignment);
      return aligned_offset >= item.first && (aligned_offset - item.first) >= size;
   });
   if (it == m_free_list.end()) {
      return std::nullopt;
   }

   if (it->second == size) {
      const auto offset = it->first;
      m_free_list.erase(it);
      return offset;
   }

   const auto mod = (it->first + it->second) % alignment;
   if (mod != 0) {
      m_free_list.emplace(it->first + it->second - mod, mod);
   }

   const auto total_size = size + mod;
   it->second -= total_size;
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

Area HeapAllocator::allocated_area() const
{
   if (m_free_list.empty()) {
      return {.size = 0, .offset = 0};
   }

   auto [first_off, first_size] = *m_free_list.begin();
   if (m_free_list.size() == 1) {
      return {.size = m_size - first_size, .offset = first_off + first_size};
   }

   auto [last_off, last_size] = *std::prev(m_free_list.end());

   return {.size = last_off - (first_off + first_size), .offset = first_off + first_size};
}

}// namespace triglav::memory
