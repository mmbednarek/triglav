#define TG_HEAP_ALLOCATOR_TEST 1
#include "triglav/memory/HeapAllocator.hpp"

#include <gtest/gtest.h>
#include <random>
#include <vector>

using triglav::u32;
using triglav::memory::HeapAllocator;

TEST(HeapAllocatorTest, Default)
{
   std::mt19937 rng(2000);
   std::uniform_int_distribution dist(1, 1024);
   std::uniform_int_distribution condDist(0, 3);
   std::vector<triglav::memory::Area> allocations;

   HeapAllocator allocator{1 << 14};

   for (u32 i = 0; i < 20; ++i) {
      const auto size = dist(rng);
      auto offset = allocator.allocate(size);
      if (!offset.has_value()) {
         return;
      }

      allocations.emplace_back(size, *offset);

      if (condDist(rng) == 0 && !allocations.empty()) {
         std::uniform_int_distribution<size_t> allocRange(0, allocations.size() - 1);
         auto it = allocations.begin() + allocRange(rng);
         allocator.free(*it);
         allocations.erase(it);
      }

      // Verify integrity
      triglav::MemorySize totalSize{};
      for (const auto& [offset, size] : allocator.free_list()) {
         totalSize += size;
      }
      for (const auto& area : allocations) {
         totalSize += area.size;
      }

      ASSERT_EQ(totalSize, 1ull << 14);

      // Verify there is no continuity between free list items
      triglav::MemorySize lastOffset{~0ull};
      for (const auto& [offset, size] : allocator.free_list()) {
         ASSERT_NE(lastOffset, offset);
         lastOffset = offset + size;
      }
   }
}