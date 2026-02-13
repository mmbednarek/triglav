#include "triglav/UpdateList.hpp"
#include "triglav/testing_core/GTest.hpp"

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

   void set_object(const u32 dst_index, const TItem item)
   {
      insertions.emplace_back(dst_index, item);
   }

   void move_object(u32 src_index, u32 dst_index)
   {
      removals.emplace_back(src_index, dst_index);
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
   triglav::UpdateList<u32, Item> update_list;

   update_list.add_or_update(0, Item::Foo);
   update_list.add_or_update(1, Item::Car);
   update_list.write_to_buffers(writer);
   writer.commit();

   update_list.remove(0);
   update_list.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(update_list.top_index(), 1u);
   ASSERT_EQ(writer.items[0], Item::Car);
}

TEST(UpdateList, BasicUpdate)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> update_list;

   update_list.add_or_update(0, Item::Foo);
   update_list.add_or_update(1, Item::Car);
   update_list.write_to_buffers(writer);
   writer.commit();

   update_list.add_or_update(1, Item::Bar);
   update_list.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(update_list.top_index(), 2u);
   ASSERT_EQ(writer.items[0], Item::Foo);
   ASSERT_EQ(writer.items[1], Item::Bar);
}

TEST(UpdateList, MultipleAddsAndRemovals)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> update_list;

   update_list.add_or_update(0, Item::Foo);
   update_list.add_or_update(1, Item::Bar);
   update_list.add_or_update(2, Item::Car);
   update_list.add_or_update(3, Item::Goo);
   update_list.write_to_buffers(writer);
   writer.commit();

   update_list.remove(1);
   update_list.remove(2);
   update_list.add_or_update(4, Item::Doo);
   update_list.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(update_list.top_index(), 3u);
   ASSERT_EQ(writer.items[0], Item::Foo);
   ASSERT_EQ(writer.items[1], Item::Doo);
   ASSERT_EQ(writer.items[2], Item::Goo);
}

TEST(UpdateList, RemoveAllAddOne)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> update_list;

   update_list.add_or_update(3, Item::Foo);
   update_list.add_or_update(2, Item::Bar);
   update_list.add_or_update(1, Item::Car);
   update_list.add_or_update(0, Item::Goo);
   update_list.write_to_buffers(writer);
   writer.commit();

   update_list.remove(0);
   update_list.remove(1);
   update_list.remove(2);
   update_list.remove(3);
   update_list.add_or_update(4, Item::Doo);
   update_list.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(update_list.top_index(), 1u);
   ASSERT_EQ(writer.items[0], Item::Doo);
}

TEST(UpdateList, ConflicingRemove)
{
   UpdateWriter<> writer;
   writer.items.resize(100);
   triglav::UpdateList<u32, Item> update_list;

   update_list.add_or_update(0, Item::Foo);
   update_list.add_or_update(1, Item::Bar);
   update_list.add_or_update(2, Item::Car);
   update_list.add_or_update(3, Item::Goo);
   update_list.write_to_buffers(writer);
   writer.commit();

   update_list.remove(1);
   update_list.remove(2);
   update_list.write_to_buffers(writer);
   writer.commit();

   ASSERT_EQ(update_list.top_index(), 2u);
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

   triglav::UpdateList<u32, int> update_list;

   std::set<int> expected_values;
   scope(INITIAL_SETUP)
   {
      for (int i = 0; i < 500; ++i) {
         expected_values.emplace(range0(gen));
      }

      initial_count = expected_values.size();

      u32 index = 0;
      for (const auto value : expected_values) {
         update_list.add_or_update(index++, int{value});
      }
   }

   update_list.write_to_buffers(writer);
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
            update_list.add_or_update(add_index++, int{value});
            expected_values.emplace(value);
         }
      }

      scope(INSERT_REMOVALS)
      {
         std::uniform_int_distribution<int> removal_range(0, static_cast<int>(initial_count) - 1);

         std::set<int> removals;
         for (int i = 0; i < 100; ++i) {
            auto index = removal_range(gen);
            if (update_list.key_map().contains(index)) {
               removals.emplace(index);
            }
         }
         removal_count += removals.size();

         for (const auto rem : removals) {
            update_list.remove(rem);
            expected_values.erase(writer.items[update_list.key_map().at(rem)]);
         }
      }

      update_list.write_to_buffers(writer);
      writer.commit();
   }

   ASSERT_EQ(update_list.top_index(), initial_count + addition_count - removal_count);

   std::set<int> actual_values;
   for (size_t i = 0; i < update_list.top_index(); ++i) {
      actual_values.emplace(writer.items[i]);
   }

   ASSERT_EQ(expected_values, actual_values);
}
