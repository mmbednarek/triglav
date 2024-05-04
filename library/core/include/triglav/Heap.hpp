#pragma once

#include <vector>
#include <utility>
#include <stdexcept>
#include <algorithm>

#include "Int.hpp"

namespace triglav {

template<typename TKey, typename TValue>
class Heap {
 public:
   using Pair = std::pair<TKey, TValue>;

   template<typename ...TArgs>
   void emplace(TKey key, TArgs&&... args) {
      m_built = false;
      m_staging.emplace_back(key, TValue{std::forward<TArgs>(args)...});
   }

   TValue& operator[](const TKey key) {
      if (not m_built) {
         throw std::runtime_error("heap: not constructed");
      }

      u32 base = 0;
      u32 extent = 0;
      u32 sum = 0;
      while (sum < m_heap.size()) {
         auto& value = m_heap[sum];
         if (not value.has_value()) {
            throw std::runtime_error("heap: key not found");
         }
         if (value->first == key) {
            return value->second;
         }

         base = (base << 1) | 1;
         extent = (extent << 1) | ((value->first < key) ? 1 : 0);
         sum = base + extent;
      }

      throw std::runtime_error("heap: key not found");
   }

   void make_heap() {
      if (m_staging.empty()) {
         m_built = true;
         return;
      }

      std::sort(m_staging.begin(), m_staging.end(), [](const Pair& left, const Pair& right) {
         return left.first < right.first;
      });

      m_size = m_staging.size();

      u32 heapSize = 1;
      while (heapSize < m_size) {
         heapSize = (heapSize << 1) | 1;
      }

      m_heap.resize(heapSize);

      make_heap_internal(0, 0, 0, m_size - 1);

      m_built = true;
      m_staging.clear();
   }

   [[nodiscard]] u32 size() const {
      return m_size;
   }

   [[nodiscard]] auto begin() {
      return m_heap.begin();
   }

   [[nodiscard]] auto end() {
      return m_heap.end();
   }

   void clear() {
      m_built = false;
      m_size = 0;
      m_staging.clear();
      m_heap.clear();
   }

 private:
   void make_heap_internal(const u32 base, const u32 extent, const u32 start, const u32 end) {
      if (end == start) {
         m_heap[base + extent] = std::move(m_staging[start]);
         return;
      }

      const u32 mid = (end - start)/2 + start;
      m_heap[base + extent] = std::move(m_staging[mid]);

      const u32 newBase = (base << 1) | 1;
      const u32 newExtent = extent << 1;

      if (mid != start) {
         make_heap_internal(newBase, newExtent, start, mid - 1);
      }
      make_heap_internal(newBase, newExtent + 1, mid + 1, end);
   }

   bool m_built{false};
   u32 m_size{};
   std::vector<std::pair<TKey, TValue>> m_staging;
   std::vector<std::optional<std::pair<TKey, TValue>>> m_heap;
};

}