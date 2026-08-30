#include "triglav/io/BufferReader.hpp"
#include "triglav/io/DynamicWriter.hpp"
#include "triglav/testing_core/GTest.hpp"
#include "triglav/world/ComponentManager.hpp"
#include "triglav/world/ComponentStorage.hpp"
#include "triglav/world/EntityStorage.hpp"
#include "triglav/world/HierarchyTree.hpp"
#include "triglav/world/Level.hpp"
#include "triglav/world/World.hpp"

#include <random>

using triglav::Transform3D;
using triglav::world::calc_component_stride_and_offset;
using triglav::world::ComponentManager;
using triglav::world::ComponentStorage;
using triglav::world::EntityID;
using triglav::world::EntityStorage;
using triglav::world::HierarchyTree;
using triglav::world::Level;

using namespace triglav::name_literals;

TEST(ComponentTest, Simple)
{
   triglav::world::ensure_component_registration();

   const auto tid = ComponentManager::the().component_id_by_class_name("triglav::Transform3D"_name);
   ASSERT_TRUE(tid.has_value());

   ComponentStorage storage(*tid);
   const auto id0 = storage.allocate_component(0);
   const auto id1 = storage.allocate_component(1);

   void* ptr0 = storage.get_component(id0);
   void* ptr1 = storage.get_component(id1);
   ASSERT_NE(ptr0, ptr1);

   Transform3D& transform = *static_cast<Transform3D*>(storage.get_component(id0));
   transform = Transform3D::identity();

   ASSERT_EQ(storage.get_component_by_entity_id(0), ptr0);
   ASSERT_EQ(storage.get_component_by_entity_id(1), ptr1);

   const auto [eid0, cptr0] = storage.get_component_with_eid(id0);
   const auto [eid1, cptr1] = storage.get_component_with_eid(id1);
   ASSERT_EQ(cptr0, ptr0);
   ASSERT_EQ(cptr1, ptr1);
   ASSERT_EQ(eid0, 0u);
   ASSERT_EQ(eid1, 1u);

   triglav::u32 i = 0;
   for (const auto [entity_id, ptr] : storage) {
      if (i == 0) {
         ASSERT_EQ(entity_id, 0u);
         ASSERT_EQ(ptr, ptr0);
      } else {
         ASSERT_EQ(entity_id, 1u);
         ASSERT_EQ(ptr, ptr1);
      }
      ++i;
   }
}

TEST(ComponentTest, StrideTest)
{
   // Special cases
   ASSERT_EQ(calc_component_stride_and_offset(12, 8), std::pair(16, 12));
   ASSERT_EQ(calc_component_stride_and_offset(24, 16), std::pair(32, 28));
   ASSERT_EQ(calc_component_stride_and_offset(6, 4), std::pair(12, 8));
   ASSERT_EQ(calc_component_stride_and_offset(6, 2), std::pair(12, 8));
   ASSERT_EQ(calc_component_stride_and_offset(6, 1), std::pair(12, 8));
   ASSERT_EQ(calc_component_stride_and_offset(1, 1), std::pair(8, 4));

   // Random values
   std::random_device rd;
   std::uniform_int_distribution<int> dist_alignment(0, 8);
   std::uniform_int_distribution<int> dist_size(2, 256);
   for (int i = 0; i < 128; ++i) {
      const auto align = 1 << dist_alignment(rd);
      std::uniform_int_distribution<int> dist_add(0, align);
      const auto size = align * dist_size(rd) + dist_add(rd);

      const auto [got_stride, got_offset] = calc_component_stride_and_offset(size, align);
      ASSERT_TRUE((got_stride - got_offset) >= sizeof(triglav::u32));
      ASSERT_EQ(got_offset % sizeof(triglav::u32), 0u);
   }
}

TEST(ComponentTest, EntityStorage)
{
   EntityStorage storage;
   storage.init();

   const auto cid = ComponentManager::the().component_id_by_class_name("triglav::Transform3D"_name);
   ASSERT_TRUE(cid.has_value());

   std::array<EntityID, 3> entities{};
   entities[0] = storage.allocate_entity(0);
   entities[1] = storage.allocate_entity(0);
   entities[2] = storage.allocate_entity(0);

   storage.allocate_component(0, *cid);
   storage.allocate_component(1, *cid);
   storage.allocate_component(2, *cid);

   const auto ptr1 = static_cast<Transform3D*>(storage.get_component(entities[0], *cid));
   const auto ptr2 = static_cast<Transform3D*>(storage.get_component(entities[1], *cid));
   const auto ptr3 = static_cast<Transform3D*>(storage.get_component(entities[2], *cid));

   ASSERT_NE(ptr1, ptr2);
   ASSERT_NE(ptr2, ptr3);
}

TEST(ComponentTest, Level)
{
   Level level;
   level.init_entities();

   const auto foo = level.new_entity();
   const auto bar = level.new_entity();

   Transform3D* foo_trans = level.insert_component<Transform3D>(foo);
   ASSERT_NE(foo_trans, nullptr);

   triglav::world::Tag* foo_tag = level.insert_component<triglav::world::Tag>(foo);
   ASSERT_NE(foo_tag, nullptr);

   Transform3D* bar_trans = level.insert_component<Transform3D>(bar);
   ASSERT_NE(bar_trans, nullptr);

   ASSERT_NE(foo_trans, bar_trans);

   ASSERT_EQ(foo_trans, &level.component<Transform3D>(foo));
   ASSERT_EQ(bar_trans, &level.component<Transform3D>(bar));

   auto [trans, tag] = level.components<Transform3D, triglav::world::Tag>(foo);
   ASSERT_EQ(&trans, foo_trans);
   ASSERT_EQ(&tag, foo_tag);

   bool has_foo = false;
   bool has_bar = false;
   for (const auto [entity_id, comp] : level.all<Transform3D>()) {
      if (&comp == foo_trans) {
         ASSERT_EQ(entity_id, foo);
         has_foo = true;
      }
      if (&comp == bar_trans) {
         ASSERT_EQ(entity_id, bar);
         has_bar = true;
      }
   }

   ASSERT_TRUE(has_foo);
   ASSERT_TRUE(has_bar);
}

TEST(ComponentTest, ComponentStorageSerialization)
{
   const auto tid = ComponentManager::the().component_id_by_class_name("triglav::Transform3D"_name);
   ASSERT_TRUE(tid.has_value());

   ComponentStorage storage(*tid);
   const auto id0 = storage.allocate_component(0);
   const auto id1 = storage.allocate_component(1);

   Transform3D& t1 = *static_cast<Transform3D*>(storage.get_component(id0));
   t1 = Transform3D::identity();
   t1.translation = triglav::Vector3{1, 2, 3};

   Transform3D& t2 = *static_cast<Transform3D*>(storage.get_component(id1));
   t2 = Transform3D::identity();
   t2.translation = triglav::Vector3{4, 5, 6};

   triglav::io::DynamicWriter writer;
   ASSERT_TRUE(storage.serialize(writer));

   triglav::io::BufferReader reader(writer.span());

   ComponentStorage read_storage;
   ASSERT_TRUE(read_storage.deserialize(reader));

   Transform3D& r1 = *static_cast<Transform3D*>(read_storage.get_component_by_entity_id(0));
   ASSERT_EQ(r1.translation, t1.translation);
   ASSERT_EQ(r1.scale, t1.scale);
   ASSERT_EQ(r1.rotation, t1.rotation);

   Transform3D& r2 = *static_cast<Transform3D*>(read_storage.get_component_by_entity_id(1));
   ASSERT_EQ(r2.translation, t2.translation);
   ASSERT_EQ(r2.scale, t2.scale);
   ASSERT_EQ(r2.rotation, t2.rotation);
}

TEST(ComponentTest, HierarchyTest)
{
   HierarchyTree tree;
   tree.add_child(0, 1);
   tree.add_child(0, 2);
   tree.add_child(0, 3);

   tree.add_child(1, 5);
   tree.add_child(1, 6);

   tree.add_child(5, 7);
   tree.add_child(5, 8);

   tree.add_child(3, 9);
   tree.add_child(3, 10);

   std::array<EntityID, 3> entities{};
   auto children_of_0 = tree.children_of(0);
   std::copy(children_of_0.begin(), children_of_0.end(), entities.begin());

   std::array<EntityID, 3> expected_entities{3, 2, 1};
   ASSERT_EQ(entities, expected_entities);

   ASSERT_EQ(tree.parent_of(1), 0u);
   ASSERT_EQ(tree.parent_of(5), 1u);
   ASSERT_EQ(tree.parent_of(8), 5u);

   tree.remove_entity(1);

   std::array<EntityID, 4> entities2{};
   children_of_0 = tree.children_of(0);
   std::copy(children_of_0.begin(), children_of_0.end(), entities2.begin());

   std::array<EntityID, 4> expected_entities2{3, 2, 6, 5};
   ASSERT_EQ(entities2, expected_entities2);

   std::array<EntityID, 2> entities3{};
   auto children_of_5 = tree.children_of(5);
   std::copy(children_of_5.begin(), children_of_5.end(), entities3.begin());

   std::array<EntityID, 2> expected_entities3{8, 7};
   ASSERT_EQ(entities3, expected_entities3);

   tree.remove_entity(8);

   std::array<EntityID, 1> entities4{};
   children_of_5 = tree.children_of(5);
   std::copy(children_of_5.begin(), children_of_5.end(), entities4.begin());

   std::array<EntityID, 1> expected_entities4{7};
   ASSERT_EQ(entities4, expected_entities4);

   tree.remove_tree(5);

   std::array<EntityID, 3> entities5{};
   children_of_0 = tree.children_of(0);
   std::copy(children_of_0.begin(), children_of_0.end(), entities5.begin());

   std::array<EntityID, 3> expected_entities5{3, 2, 6};
   ASSERT_EQ(entities5, expected_entities5);
}
