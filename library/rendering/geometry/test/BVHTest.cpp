#include "triglav/geometry/BVHTree.hpp"
#include "triglav/testing_core/GTest.hpp"

#include <string>

using triglav::Vector3;
using triglav::geometry::BoundingBox;
using triglav::geometry::BVHTree;

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
};

TEST(BVHTest, BasicTree)
{
   BVHTree<std::string, Object> tree;

   std::array<Object, 4> objects{
      Object{"A", {-1, -1, -1}, {1, 1, 1}},
      Object{"B", {-4, -4, -4}, {-2, -2, -2}},
      Object{"C", {1.5, 0, 0}, {3, 1, 1}},
      Object{"D", {0, -5, 5}, {1, -3, 8}},
   };
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
      {-0.5, -0.5, 0.25},
      {0.5, 0.25, 0.5},
   });

   const auto added_hit = tree.traverse({.origin = {0.0, 0, 0.2}, .direction = {0, 0, 1}, .distance = 10.0f});
   EXPECT_NE(added_hit.payload, nullptr);
   EXPECT_EQ(added_hit.payload->label, "E");
   EXPECT_EQ(&tree.get("E"), added_hit.payload);

   tree.remove("B");

   const auto removed_hit = tree.traverse({.origin = {-5, -3, -3}, .direction = {1, 0, 0}, .distance = 10.0f});
   EXPECT_EQ(removed_hit.payload, nullptr);
}