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
};

TEST(BVHTest, BasicTree)
{
   BVHTree<Object> tree;

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

   const auto miss = tree.traverse({.origin = {-5, 0, 0}, .direction = {-1, 0, 0}, .distance = 10.0f});
   EXPECT_EQ(miss.distance, INFINITY);
   EXPECT_EQ(miss.payload, nullptr);
}