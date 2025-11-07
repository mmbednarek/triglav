#include "triglav/Math.hpp"

#include <gtest/gtest.h>
#include <random>

using triglav::g_pi;
using triglav::Quaternion;
using triglav::Transform3D;
using triglav::Vector3;
using triglav::Vector4;

namespace {

[[nodiscard]] bool float_equals(const float a, const float b)
{
   return std::abs(a - b) < 0.001f;
}

[[nodiscard]] bool quat_equals(const Quaternion a, const Quaternion b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z) && float_equals(a.w, b.w);
}

[[nodiscard]] bool vec3_equals(const Vector3 a, const Vector3 b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z);
}

}// namespace

TEST(MathTest, BasicCase)
{
   std::mt19937 rng{1111};
   std::uniform_real_distribution<float> dist{-2, 2};

   for (auto i = 0; i < 100; ++i) {
      Transform3D transform{
         .rotation = glm::normalize(Quaternion({dist(rng) * g_pi, dist(rng) * g_pi, dist(rng) * g_pi})),
         .scale = Vector3{0.1f + abs(dist(rng)), 0.1f + abs(dist(rng)), 0.1f + abs(dist(rng))},
         .translation = Vector3{dist(rng), dist(rng), dist(rng)},
      };
      transform.rotation = transform.rotation / triglav::sign(transform.rotation.w);

      const auto mat = transform.to_matrix();
      const auto decodedTransform = Transform3D::from_matrix(mat);

      EXPECT_TRUE(quat_equals(decodedTransform.rotation, transform.rotation));
      EXPECT_TRUE(vec3_equals(decodedTransform.scale, transform.scale));
      EXPECT_TRUE(vec3_equals(decodedTransform.translation, transform.translation));
   }
}

TEST(MathTest, ClosestPoint)
{
   const auto result =
      triglav::find_closest_point_between_lines({0, 0, 0}, glm::normalize(Vector3{1, 1, 0}), {1, 0, 0}, glm::normalize(Vector3{-1, 1, 0}));
   EXPECT_EQ(result, Vector3(0.5, 0.5, 0));
}

TEST(MathTest, ClosestPointToLine)
{
   const auto result = triglav::find_closest_point_on_line({0, 0, 0}, glm::normalize(Vector3{1, 1, 0}), Vector3{1, 2, 0});
   EXPECT_TRUE(glm::dot(result - Vector3(1.5, 1.5, 0), Vector3{1, 1, 1}) < 0.001f);
}
