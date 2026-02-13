#define TG_HEAP_ALLOCATOR_TEST 1
#include "triglav/memory/HeapAllocator.hpp"
#include "triglav/testing_core/GTest.hpp"

#include <random>
#include <vector>

using triglav::u32;
using triglav::memory::HeapAllocator;

TEST(HeapAllocatorTest, Default)
{
   std::mt19937 rng(2000);
   std::uniform_int_distribution dist(1, 1024);
   std::uniform_int_distribution cond_dist(0, 3);
   std::vector<triglav::memory::Area> allocations;

   HeapAllocator allocator{1 << 14};

   for (u32 i = 0; i < 20; ++i) {
      const auto size = dist(rng);
      auto offset = allocator.allocate(size);
      if (!offset.has_value()) {
         return;
      }

      allocations.emplace_back(size, *offset);

      if (cond_dist(rng) == 0 && !allocations.empty()) {
         std::uniform_int_distribution<size_t> alloc_range(0, allocations.size() - 1);
         auto it = allocations.begin() + alloc_range(rng);
         allocator.free(*it);
         allocations.erase(it);
      }

      // Verify integrity
      triglav::MemorySize total_size{};
      for (const auto& [_, item_size] : allocator.free_list()) {
         total_size += item_size;
      }
      for (const auto& area : allocations) {
         total_size += area.size;
      }

      ASSERT_EQ(total_size, 1ull << 14);

      // Verify there is no continuity between free list items
      triglav::MemorySize last_offset{~0ull};
      for (const auto& [item_offset, item_size] : allocator.free_list()) {
         ASSERT_NE(last_offset, item_offset);
         last_offset = item_offset + item_size;
      }
   }

   for (const auto& alloc : allocations) {
      allocator.free(alloc);
   }

   ASSERT_EQ(allocator.free_list().size(), 1ull);
   ASSERT_EQ(allocator.free_list().begin()->first, 0ull);
   ASSERT_EQ(allocator.free_list().begin()->second, 1ull << 14);
}