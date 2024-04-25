#include <gtest/gtest.h>

#include <map>
#include <chrono>

#include "triglav/Heap.hpp"

TEST(HeapTest, NotFull)
{
   triglav::Heap<int, std::string> values{};

   values.emplace(100, "hello");
   values.emplace(20, "hi");
   values.emplace(2137, "foo");
   values.emplace(531, "bar");
   values.emplace(8, "rrrr");
   values.emplace(6868, "aaaaa");

   values.make_heap();

   EXPECT_EQ(values[100], "hello");
   EXPECT_EQ(values[20], "hi");
   EXPECT_EQ(values[2137], "foo");
   EXPECT_EQ(values[531], "bar");
   EXPECT_EQ(values[8], "rrrr");
   EXPECT_EQ(values[6868], "aaaaa");
}

TEST(HeapTest, FullCapacity)
{
   triglav::Heap<int, std::string> values{};

   values.emplace(100, "hello");
   values.emplace(20, "hi");
   values.emplace(2137, "foo");
   values.emplace(531, "bar");
   values.emplace(8, "rrrr");
   values.emplace(6868, "aaaaa");
   values.emplace(2222, "bbbbb");

   values.make_heap();

   EXPECT_EQ(values[100], "hello");
   EXPECT_EQ(values[20], "hi");
   EXPECT_EQ(values[2137], "foo");
   EXPECT_EQ(values[531], "bar");
   EXPECT_EQ(values[8], "rrrr");
   EXPECT_EQ(values[6868], "aaaaa");
   EXPECT_EQ(values[2222], "bbbbb");
}

TEST(HeapTest, LargeAmount)
{
   triglav::Heap<int, int> values{};

   srand(2137);

   for (int i = 0; i < 10000; ++i) {
      const auto key   = rand();
      const auto value = rand();
      values.emplace(key, value);
   }

   values.make_heap();

   srand(2137);

   const auto beg = std::chrono::steady_clock::now();
   for (int i = 0; i < 10000; ++i) {
      const auto key   = rand();
      const auto value = rand();
      EXPECT_EQ(values[key], value);
   }
   const auto end = std::chrono::steady_clock::now();
   std::cout << "Time :" << (end - beg) << '\n';
}

TEST(HeapTest, CompareMap)
{
   std::map<int, int> values{};

   srand(2137);

   for (int i = 0; i < 10000; ++i) {
      const auto key   = rand();
      const auto value = rand();
      values.emplace(key, value);
   }

   srand(2137);

   const auto beg = std::chrono::steady_clock::now();
   for (int i = 0; i < 10000; ++i) {
      const auto key   = rand();
      const auto value = rand();
      EXPECT_EQ(values[key], value);
   }
   const auto end = std::chrono::steady_clock::now();
   std::cout << "Time :" << (end - beg) << '\n';
}
