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
   static constexpr auto aquiredIndex = std::numeric_limits<u32>::max();

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
      m_path[m_head] = aquiredIndex;
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

      if (m_path[index] != aquiredIndex) {
         return false;
      }

      m_path[index] = m_head;
      m_head = index;

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

   u32 m_chain{aquiredIndex};

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
      const TObject* basePtr;

      bool operator<(const PointerRange& other) const
      {
         return (basePtr + CBucketSize) < other.basePtr;
      }

      bool operator==(const PointerRange& other) const
      {
         return (other.basePtr >= basePtr) && (other.basePtr < (basePtr + CBucketSize));
      }
   };

 public:
   using Bucket = PoolBucket<TObject, TFactory, CBucketSize>;

   explicit ObjectPool(TFactory& factory = default_constructor<TObject>) :
       m_objectFactory(factory)
   {
   }

   TObject* acquire_object()
   {
      if (m_bucketHead == Bucket::aquiredIndex) {
         m_bucketHead = m_buckets.size();
         auto& bucket = m_buckets.emplace_back(std::make_unique<Bucket>(m_objectFactory));
         m_bucketRanges[PointerRange{bucket->base_ptr()}] = m_bucketHead;
         return bucket->acquire_object();
      }

      auto& bucket = m_buckets[m_bucketHead];
      auto* result = bucket->acquire_object();
      if (bucket->is_full()) {
         m_bucketHead = bucket->m_chain;
         bucket->m_chain = Bucket::aquiredIndex;
      }

      return result;
   }

   bool release_object(const TObject* obj)
   {
      auto it = m_bucketRanges.find(PointerRange{obj});
      if (it == m_bucketRanges.end()) {
         return false;
      }

      assert(it->second < m_buckets.size());

      auto& bucket = m_buckets[it->second];

      if (bucket->is_full()) {
         bucket->m_chain = m_bucketHead;
         m_bucketHead = it->second;
      }

      // TODO: Release unused buckets (?)

      return bucket->release_object(obj);
   }

 private:
   TFactory& m_objectFactory;
   std::vector<std::unique_ptr<Bucket>> m_buckets;
   std::map<PointerRange, u32> m_bucketRanges;
   u32 m_bucketHead{Bucket::aquiredIndex};
};

}// namespace triglav