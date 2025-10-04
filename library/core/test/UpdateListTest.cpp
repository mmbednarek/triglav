#include "triglav/UpdateList.hpp"

#include <gtest/gtest.h>
#include <random>

using triglav::u32;

enum class Item
{
   Foo,
   Bar,
   Car,
   Goo,
   Doo,
};

template<typename TItem = Item>
struct UpdateWriter
{
   std::vector<std::pair<u32, TItem>> insertions;
   std::vector<std::pair<u32, u32>> removals;
   std::vector<TItem> items;

   void add_insertion(u32 index, TItem item)
   {
      insertions.emplace_back(index, item);
   }

   void add_removal(u32 src, u32 dst)
   {
      removals.emplace_back(src, dst);
   }

   void commit()
   {
      for (const auto [src, dst] : removals) {
         items[dst] = items[src];
      }
      removals.clear();

      for (const auto [index, item] : insertions) {
         items[index] = item;
      }
      insertions.clear();
   }
};

TEST(UpdateList, BasicAddAndRemove)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> updateList;

   updateList.add_or_update(0, Item::Foo);
   updateList.add_or_update(1, Item::Car);
   updateList.write_to_buffers(writer);
   writer.commit();

   updateList.remove(0);
   updateList.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(updateList.top_index(), 1u);
   ASSERT_EQ(writer.items[0], Item::Car);
}

TEST(UpdateList, BasicUpdate)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> updateList;

   updateList.add_or_update(0, Item::Foo);
   updateList.add_or_update(1, Item::Car);
   updateList.write_to_buffers(writer);
   writer.commit();

   updateList.add_or_update(1, Item::Bar);
   updateList.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(updateList.top_index(), 2u);
   ASSERT_EQ(writer.items[0], Item::Foo);
   ASSERT_EQ(writer.items[1], Item::Bar);
}

TEST(UpdateList, MultipleAddsAndRemovals)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> updateList;

   updateList.add_or_update(0, Item::Foo);
   updateList.add_or_update(1, Item::Bar);
   updateList.add_or_update(2, Item::Car);
   updateList.add_or_update(3, Item::Goo);
   updateList.write_to_buffers(writer);
   writer.commit();

   updateList.remove(1);
   updateList.remove(2);
   updateList.add_or_update(4, Item::Doo);
   updateList.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(updateList.top_index(), 3u);
   ASSERT_EQ(writer.items[0], Item::Foo);
   ASSERT_EQ(writer.items[1], Item::Doo);
   ASSERT_EQ(writer.items[2], Item::Goo);
}

TEST(UpdateList, RemoveAllAddOne)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> updateList;

   updateList.add_or_update(3, Item::Foo);
   updateList.add_or_update(2, Item::Bar);
   updateList.add_or_update(1, Item::Car);
   updateList.add_or_update(0, Item::Goo);
   updateList.write_to_buffers(writer);
   writer.commit();

   updateList.remove(0);
   updateList.remove(1);
   updateList.remove(2);
   updateList.remove(3);
   updateList.add_or_update(4, Item::Doo);
   updateList.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(updateList.top_index(), 1u);
   ASSERT_EQ(writer.items[0], Item::Doo);
}

TEST(UpdateList, ConflicingRemove)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> updateList;

   updateList.add_or_update(0, Item::Foo);
   updateList.add_or_update(1, Item::Bar);
   updateList.add_or_update(2, Item::Car);
   updateList.add_or_update(3, Item::Goo);
   updateList.write_to_buffers(writer);
   writer.commit();

   updateList.remove(1);
   updateList.remove(2);
   updateList.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(updateList.top_index(), 2u);
   ASSERT_EQ(writer.items[0], Item::Foo);
   ASSERT_EQ(writer.items[1], Item::Goo);
}

#define scope(...)

TEST(UpdateList, RandomList)
{
   UpdateWriter<int> writer;
   writer.items.resize(1000);

   std::mt19937 gen(1111);
   std::uniform_int_distribution<int> range0(0, 999);

   size_t initial_count{};
   size_t addition_count{};
   size_t removal_count{};

   triglav::UpdateList<u32, int> updateList;

   std::set<int> expected_values;
   scope(INITIAL_SETUP)
   {
      for (int i = 0; i < 500; ++i) {
         expected_values.emplace(range0(gen));
      }

      initial_count = expected_values.size();

      u32 index = 0;
      for (const auto value : expected_values) {
         updateList.add_or_update(index++, int{value});
      }
   }

   updateList.write_to_buffers(writer);
   writer.commit();

   scope(CHECK_INTIAL_VALUES)
   {
      size_t index{};
      for (const auto value : expected_values) {
         ASSERT_EQ(writer.items[index++], value);
      }
   }

   u32 add_index = 1000;
   for (int repeat = 0; repeat < 10; ++repeat) {
      scope(INSERT_ADDITIONS)
      {
         std::uniform_int_distribution<int> addition_range(1000, 1999);

         std::set<int> additions;
         for (int i = 0; i < 100; ++i) {
            additions.emplace(addition_range(gen));
         }
         addition_count += additions.size();

         for (const auto value : additions) {
            updateList.add_or_update(add_index++, int{value});
            expected_values.emplace(value);
         }
      }

      scope(INSERT_REMOVALS)
      {
         std::uniform_int_distribution<int> removal_range(0, initial_count-1);

         std::set<int> removals;
         for (int i = 0; i < 100; ++i) {
            auto index = removal_range(gen);
            if (updateList.key_map().contains(index)) {
                removals.emplace(index);
            }
         }
         removal_count += removals.size();

         for (const auto rem : removals) {
            updateList.remove(rem);
            expected_values.erase(writer.items[updateList.key_map().at(rem)]);
         }
      }

      updateList.write_to_buffers(writer);
      writer.commit();
   }

   ASSERT_EQ(updateList.top_index(), initial_count + addition_count - removal_count);

   std::set<int> actual_values;
   for (size_t i = 0; i < updateList.top_index(); ++i) {
      actual_values.emplace(writer.items[i]);
   }

   ASSERT_EQ(expected_values, actual_values);
}
