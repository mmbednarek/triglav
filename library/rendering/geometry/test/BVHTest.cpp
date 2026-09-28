#include "triglav/geometry/BVHTree.hpp"
#include "triglav/testing_core/GTest.hpp"

#include <string>

using triglav::Matrix4x4;
using triglav::Transform3D;
using triglav::Vector3;
using triglav::geometry::BottomLevelHit;
using triglav::geometry::BoundingBox;
using triglav::geometry::Ray;
using triglav::geometry::TopLevelBVH;

struct Object
{
   std::string_view label;
   BoundingBox bbox;

   Object(std::string_view label, const Vector3 min, const Vector3 max) :
       label(std::move(label)),
       bbox(min, max)
   {
   }

   const BoundingBox& bounding_box() const
   {
      return bbox;
   }

   [[nodiscard]] std::string index() const
   {
      return std::string{label};
   }

   [[nodiscard]] Matrix4x4 inv_transform_matrix() const
   {
      return Matrix4x4{1};
   }
};

struct DummyBottomLevel
{
   BoundingBox bb;

   [[nodiscard]] BottomLevelHit traverse(const Ray& ray) const
   {
      const auto hit = bb.intersect(ray);
      return BottomLevelHit{
         .distance = hit.has_value() ? hit->x : INFINITY,
         .primitive_id = 0,
      };
   }
};

struct DummyProvider
{
   std::span<Object> m_objects;
   mutable std::map<std::string, DummyBottomLevel> m_dummies;

   DummyBottomLevel& get_bottom_level(const std::string& index) const
   {
      const auto dummy = m_dummies.find(index);
      if (dummy != m_dummies.end()) {
         return dummy->second;
      }

      const auto it = std::ranges::find_if(m_objects, [&index](const Object& obj) { return obj.index() == index; });
      auto [new_dummy, ok] = m_dummies.emplace(index, DummyBottomLevel{.bb = it->bounding_box()});
      return new_dummy->second;
   }
};

TEST(BVHTest, TopLevelTest)
{
   std::array<Object, 4> objects{
      Object{"A", {-1, -1, -1}, {1, 1, 1}},
      Object{"B", {-4, -4, -4}, {-2, -2, -2}},
      Object{"C", {1.5, 0, 0}, {3, 1, 1}},
      Object{"D", {0, -5, 5}, {1, -3, 8}},
   };
   DummyProvider provider{.m_objects = objects};

   TopLevelBVH<std::string, Object, DummyBottomLevel, DummyProvider> tree(provider);
   tree.build(objects);


   const auto hit = tree.traverse({.origin = {-5, -3, -3}, .direction = {1, 0, 0}, .distance = 10.0f});
   EXPECT_NE(hit.payload, nullptr);
   EXPECT_EQ(hit.payload->label, "B");
   EXPECT_EQ(&tree.get("B"), hit.payload);

   const auto miss = tree.traverse({.origin = {-5, 0, 0}, .direction = {-1, 0, 0}, .distance = 10.0f});
   EXPECT_EQ(miss.distance, INFINITY);
   EXPECT_EQ(miss.payload, nullptr);

   tree.add(Object{
      "E",
      {-0.5, -0.5, 5.1},
      {0.5, 0.5, 5.5},
   });
   provider.m_dummies["E"] = DummyBottomLevel{BoundingBox{
      {-0.5, -0.5, 5.1},
      {0.5, 0.5, 5.5},
   }};

   const auto added_hit = tree.traverse({.origin = {0.0, 0, 5.0}, .direction = {0, 0, 1}, .distance = 10.0f});
   EXPECT_NE(added_hit.payload, nullptr);
   EXPECT_EQ(added_hit.payload->label, "E");
   EXPECT_EQ(&tree.get("E"), added_hit.payload);

   tree.remove("B");

   const auto removed_hit = tree.traverse({.origin = {-5, -3, -3}, .direction = {1, 0, 0}, .distance = 10.0f});
   EXPECT_EQ(removed_hit.payload, nullptr);
}

TEST(BVHTest, MinimalVector)
{
   const BoundingBox static_bb{
      .min = {-1, -1, -1},
      .max = {1, 1, 1},
   };
   const BoundingBox dynamic_bb{
      .min = {0, 0, 0},
      .max = {0.5, 2, 2},
   };

   auto minimal_move = static_bb.minimum_translation_vector(dynamic_bb);

   const auto new_bb = dynamic_bb.transform(Transform3D{
      .rotation = {1, 0, 0, 0},
      .scale = {1, 1, 1},
      .translation = minimal_move,
   }
                                               .to_matrix());
   auto empty = static_bb.minimum_translation_vector(new_bb);
   ASSERT_EQ(empty, Vector3{});
}
