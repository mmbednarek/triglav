#include "triglav/ObjectPool.hpp"

#include <gtest/gtest.h>
#include <random>

using triglav::PoolBucket;
using triglav::ObjectPool;
using triglav::default_constructor;

TEST(PoolTest, Bucket_SingleItem)
{
   struct TestObj
   {
      int value{};
   };

   PoolBucket<TestObj, decltype(default_constructor<TestObj>), 8> bucket(default_constructor<TestObj>);


   auto* obj = bucket.aquire_object();
   ASSERT_NE(obj, nullptr);
   ASSERT_TRUE(bucket.release_object(obj));
}

TEST(PoolTest, Bucket_AllItems)
{
   struct TestObj
   {
      int value{};
   };

   PoolBucket<TestObj, decltype(default_constructor<TestObj>), 8> bucket(default_constructor<TestObj>);

   std::vector<TestObj*> objects;

   for (int i = 0; i < 8; ++i) {
      auto* obj = bucket.aquire_object();
      ASSERT_NE(obj, nullptr);
      objects.emplace_back(obj);
   }

   std::mt19937 g(100);
   std::ranges::shuffle(objects, g);

   ASSERT_EQ(bucket.aquire_object(), nullptr);

   for (const auto* obj : objects) {
      ASSERT_TRUE(bucket.release_object(obj));
   }

   for (const auto* obj : objects) {
      ASSERT_FALSE(bucket.release_object(obj));
   }
}

TEST(PoolTest, Bucket_AquireSomeRemoveSome)
{
   struct TestObj
   {
      int value{};
   };

   PoolBucket<TestObj, decltype(default_constructor<TestObj>), 8> bucket(default_constructor<TestObj>);

   std::vector<TestObj*> objects;

   for (int i = 0; i < 6; ++i) {
      auto* obj = bucket.aquire_object();
      ASSERT_NE(obj, nullptr);
      objects.emplace_back(obj);
   }

   std::mt19937 g(100);
   std::ranges::shuffle(objects, g);

   for (int i = 0; i < 4; ++i) {
      auto it = objects.begin();
      bucket.release_object(*it);
      objects.erase(it);
   }

   for (int i = 0; i < 6; ++i) {
      auto* obj = bucket.aquire_object();
      ASSERT_NE(obj, nullptr);
      objects.emplace_back(obj);
   }

   for (const auto* obj : objects) {
      ASSERT_TRUE(bucket.release_object(obj));
   }
}

TEST(PoolTest, Pool_SingleItem)
{
   struct TestObj
   {
      int value{};
   };

   ObjectPool<TestObj, decltype(default_constructor<TestObj>), 8> bucket(default_constructor<TestObj>);

   auto* obj = bucket.aquire_object();
   ASSERT_NE(obj, nullptr);
   ASSERT_TRUE(bucket.release_object(obj));
}

TEST(PoolTest, Pool_AllocateThenRelease)
{
   struct TestObj
   {
      int value{};
   };

   ObjectPool<TestObj, decltype(default_constructor<TestObj>), 8> bucket(default_constructor<TestObj>);

   std::vector<TestObj*> objects;

   for (int i = 0; i < 1000; ++i) {
      auto* obj = bucket.aquire_object();
      ASSERT_NE(obj, nullptr);
      objects.emplace_back(obj);
   }

   std::mt19937 g(100);
   std::ranges::shuffle(objects, g);

   for (const auto* obj : objects) {
      ASSERT_TRUE(bucket.release_object(obj));
   }
   for (const auto* obj : objects) {
      ASSERT_FALSE(bucket.release_object(obj));
   }
}

TEST(PoolTest, Pool_AllocateReleaseSomeThenAllocate)
{
   struct TestObj
   {
      int value{};
   };

   ObjectPool<TestObj, decltype(default_constructor<TestObj>), 8> bucket(default_constructor<TestObj>);

   std::vector<TestObj*> objects;

   for (int i = 0; i < 1000; ++i) {
      auto* obj = bucket.aquire_object();
      ASSERT_NE(obj, nullptr);
      objects.emplace_back(obj);
   }

   std::mt19937 g(100);
   std::ranges::shuffle(objects, g);

   for (int i = 0; i < 500; ++i) {
      auto it = objects.begin();
      ASSERT_TRUE(bucket.release_object(*it));
      objects.erase(it);
   }

   for (int i = 0; i < 600; ++i) {
      auto* obj = bucket.aquire_object();
      ASSERT_NE(obj, nullptr);
      objects.emplace_back(obj);
   }

   for (const auto* obj : objects) {
      ASSERT_TRUE(bucket.release_object(obj));
   }
   for (const auto* obj : objects) {
      ASSERT_FALSE(bucket.release_object(obj));
   }
}
