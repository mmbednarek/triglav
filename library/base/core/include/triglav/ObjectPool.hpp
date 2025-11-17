#pragma once

#include "Array.hpp"
#include "Int.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <vector>

namespace triglav {

template<typename TObject, typename TFactory, u32 CBucketSize>
class PoolBucket
{
 public:
   static constexpr auto aquired_index = std::numeric_limits<u32>::max();

   explicit PoolBucket(TFactory& factory) :
       m_objects(initialize_array<TObject, CBucketSize>(factory))
   {
      std::iota(m_path.begin(), m_path.end(), 1);
   }

   PoolBucket(const PoolBucket& other) = delete;
   PoolBucket& operator=(const PoolBucket& other) = delete;
   PoolBucket(PoolBucket&& other) noexcept = delete;
   PoolBucket& operator=(PoolBucket&& other) noexcept = delete;

   [[nodiscard]] TObject* acquire_object()
   {
      if (m_head >= CBucketSize) {
         return nullptr;
      }

      const auto point = m_path[m_head];
      m_path[m_head] = aquired_index;
      auto* obj = &m_objects[m_head];
      m_head = point;

      return obj;
   }

   bool release_object(const TObject* obj)
   {
      const auto index = obj - m_objects.data();
      if (index >= CBucketSize) {
         return false;
      }

      if (m_path[index] != aquired_index) {
         return false;
      }

      m_path[index] = m_head;
      m_head = static_cast<u32>(index);

      return true;
   }

   [[nodiscard]] bool has_object(const TObject* obj) const
   {
      return obj >= m_objects.data() && obj < (m_objects.data() + CBucketSize);
   }

   [[nodiscard]] bool is_full() const
   {
      return m_head == CBucketSize;
   }

   [[nodiscard]] const TObject* base_ptr() const
   {
      return m_objects.data();
   }

   u32 m_chain{aquired_index};

 private:
   std::array<TObject, CBucketSize> m_objects;
   std::array<u32, CBucketSize> m_path;
   u32 m_head{};
};

template<typename T>
auto default_constructor()
{
   return T{};
}


template<typename TObject, typename TFactory = decltype(default_constructor<TObject>), u32 CBucketSize = 32>
   requires(CBucketSize > 1)
class ObjectPool
{
   struct PointerRange
   {
      const TObject* base_ptr;

      bool operator<(const PointerRange& other) const
      {
         return (base_ptr + CBucketSize) < other.base_ptr;
      }

      bool operator==(const PointerRange& other) const
      {
         return (other.base_ptr >= base_ptr) && (other.base_ptr < (base_ptr + CBucketSize));
      }
   };

 public:
   using Bucket = PoolBucket<TObject, TFactory, CBucketSize>;

   explicit ObjectPool(TFactory& factory = default_constructor<TObject>) :
       m_object_factory(factory)
   {
   }

   TObject* acquire_object()
   {
      if (m_bucket_head == Bucket::aquired_index) {
         m_bucket_head = static_cast<u32>(m_buckets.size());
         auto& bucket = m_buckets.emplace_back(std::make_unique<Bucket>(m_object_factory));
         m_bucket_ranges[PointerRange{bucket->base_ptr()}] = m_bucket_head;
         return bucket->acquire_object();
      }

      auto& bucket = m_buckets[m_bucket_head];
      auto* result = bucket->acquire_object();
      if (bucket->is_full()) {
         m_bucket_head = bucket->m_chain;
         bucket->m_chain = Bucket::aquired_index;
      }

      return result;
   }

   bool release_object(const TObject* obj)
   {
      auto it = m_bucket_ranges.find(PointerRange{obj});
      if (it == m_bucket_ranges.end()) {
         return false;
      }

      assert(it->second < m_buckets.size());

      auto& bucket = m_buckets[it->second];

      if (bucket->is_full()) {
         bucket->m_chain = m_bucket_head;
         m_bucket_head = it->second;
      }

      // TODO: Release unused buckets (?)

      return bucket->release_object(obj);
   }

 private:
   TFactory& m_object_factory;
   std::vector<std::unique_ptr<Bucket>> m_buckets;
   std::map<PointerRange, u32> m_bucket_ranges;
   u32 m_bucket_head{Bucket::aquired_index};
};

}// namespace triglav