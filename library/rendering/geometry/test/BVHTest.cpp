#include "triglav/geometry/DebugMesh.hpp"
#include "triglav/geometry/MeshBVH.hpp"
#include "triglav/testing_core/GTest.hpp"

#include <string>

using triglav::Matrix4x4;
using triglav::Transform3D;
using triglav::Vector3;
using triglav::geometry::BoundingBox;
using triglav::geometry::MeshBVH;
using triglav::geometry::MeshData;
using triglav::geometry::Ray;

TEST(BVHTest, BasicTest)
{
   auto sphere = triglav::geometry::create_sphere(10, 10, 4.0f);
   sphere.triangulate();
   MeshData data{
      .vertex_data = sphere.to_vertex_data(),
      .bounding_box = sphere.calculate_bounding_box(),
   };

   MeshBVH tree;
   tree.build(data);

   MeshBVH::RayHit ray_hit = tree.ray_cast(Ray{.origin = {-5, 0, 0}, .direction = {1, 0, 0}, .distance = 10.0f});
   ASSERT_NE(ray_hit.primitive_id, triglav::geometry::NO_PRIMITIVE_ID);
   ASSERT_NEAR(ray_hit.distance, 1.1958f, 1e-4f);

   BoundingBox bb{
      .min = {-6.0f, -1.0f, -1.0f},
      .max = {-5.0f, 1.0f, 1.0f},
   };
   MeshBVH::SweepHit sweep_hit = tree.sweep_cast(bb, Vector3{5.0f, 0.0f, 0.0f});
   ASSERT_NE(sweep_hit.primitive_id, triglav::geometry::NO_PRIMITIVE_ID);
   ASSERT_NEAR(sweep_hit.time, 0.2391f, 1e-4f);
   Vector3 expected_normal{-1.0f, 0.0f, 0.0f};
   ASSERT_EQ(sweep_hit.normal, expected_normal);
}
