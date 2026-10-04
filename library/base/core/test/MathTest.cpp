#include "triglav/Math.hpp"
#include "triglav/testing_core/GTest.hpp"

#include <random>

using triglav::Matrix3x3;
using triglav::Matrix4x4;
using triglav::PI;
using triglav::Quaternion;
using triglav::Transform3D;
using triglav::u32;
using triglav::Vector3;
using triglav::Vector4;

namespace {

[[nodiscard]] bool float_equals(const float a, const float b)
{
   return std::abs(a - b) < 0.001f;
}

[[nodiscard]] bool vec3_equals(const Vector3 a, const Vector3 b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z);
}

[[nodiscard]] bool vec4_equals(const Vector4 a, const Vector4& b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z) && float_equals(a.w, b.w);
}

[[maybe_unused]] [[nodiscard]] bool quat_equals(const Quaternion& a, const Quaternion& b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z) && float_equals(a.w, b.w);
}

[[nodiscard]] bool mat4_equals(const Matrix4x4& a, const Matrix4x4& b)
{
   for (u32 c = 0; c < 4; ++c) {
      for (u32 r = 0; r < 4; ++r) {
         if (!float_equals(a[c][r], b[c][r]))
            return false;
      }
   }
   return true;
}

[[nodiscard]] bool mat3_equals(const Matrix3x3& a, const Matrix3x3& b)
{
   for (u32 c = 0; c < 3; ++c) {
      for (u32 r = 0; r < 3; ++r) {
         if (!float_equals(a[c][r], b[c][r]))
            return false;
      }
   }
   return true;
}

}// namespace

TEST(MathTest, Matrices)
{
   Matrix4x4 mat = Matrix4x4::identity();
   Vector4 vec{1, 2, 3, 4};
   ASSERT_EQ(mat * vec, vec);

   mat = Matrix4x4{
      {1, 2, 3, -4},
      {5, 8, 0, 4},
      {6, -9, 1, 5},
      {7, 0, 2, 5},
   };
   const auto mat_vec = mat * vec;

   Vector4 expected_vec{57, -9, 14, 39};
   ASSERT_TRUE(vec4_equals(mat_vec, expected_vec));

   Matrix4x4 inv_mat = mat.inverse();
   Matrix4x4 expected_inv_mat{
      {112.0f, 197.0f, 200.0f, -268.0f},
      {-5.0f, 10.0f, -39.0f, 27.0f},
      {-67.0f, -287.0f, -270.0f, 446.0f},
      {-130.0f, -161.0f, -172.0f, 281.0f},
   };
   expected_inv_mat /= 421.0f;

   ASSERT_TRUE(mat4_equals(inv_mat, expected_inv_mat));
   ASSERT_TRUE(mat4_equals(mat * inv_mat, Matrix4x4::identity()));

   Matrix3x3 inv_mat3 = mat.shrink().inverse();
   Matrix3x3 expected_inv_mat3{
      {-8.0f, 29.0f, 24.0f},
      {5.0f, 17.0f, -15.0f},
      {93.0f, -21.0f, 2.0f},
   };
   expected_inv_mat3 /= 281.0f;
   ASSERT_TRUE(mat3_equals(mat.shrink() * inv_mat3, Matrix3x3::identity()));
   ASSERT_TRUE(mat3_equals(inv_mat3, expected_inv_mat3));

   Matrix4x4 transposed_mat = mat.transpose();
   Matrix4x4 expected_transposed_mat = {
      {1, 5, 6, 7},
      {2, 8, -9, 0},
      {3, 0, 1, 2},
      {-4, 4, 5, 5},
   };
   ASSERT_TRUE(mat4_equals(transposed_mat, expected_transposed_mat));

   Matrix4x4 trans_mat = Matrix4x4::translation({10, 20, -50});

   Vector4 trans_vec = trans_mat * Vector4{5, 0, 10, 1};
   Vector4 expected_trans_vec = Vector4{5, 0, 10, 1} + Vector4{10, 20, -50, 0};
   ASSERT_TRUE(vec4_equals(trans_vec, expected_trans_vec));

   Matrix4x4 scale_mat = Matrix3x3::scale({2, 5, 9}).extend();
   Vector4 scale_vec = scale_mat * Vector4{5, 0, 10, 1};
   Vector4 expected_scale_vec = Vector4{Vector3{5, 0, 10} * Vector3{2, 5, 9}, 1};
   ASSERT_TRUE(vec4_equals(scale_vec, expected_scale_vec));

   Matrix4x4 rot_mat = Matrix4x4::rotation(Quaternion::from_euler_angles({PI / 4.0f, PI * 3.0f / 4.0f, PI * 3.0f / 2.0f}));
   Vector4 rot_vec = rot_mat * Vector4{1, 0, 0, 1};
   Vector4 expected_rot_vec = Vector4{0, std::sqrt(2.0f) / 2.0f, -std::sqrt(2.0f) / 2.0f, 1};
   ASSERT_TRUE(vec4_equals(rot_vec, expected_rot_vec));
}

TEST(MathTest, BasicCase)
{
   std::mt19937 rng{1111};
   std::uniform_real_distribution<float> dist{-2, 2};

   for (auto i = 0; i < 100; ++i) {
      Transform3D transform{
         .rotation = Quaternion::from_euler_angles({dist(rng) * PI, dist(rng) * PI, dist(rng) * PI}).normalize(),
         .scale = Vector3{0.1f} + Vector3{dist(rng), dist(rng), dist(rng)}.abs(),
         .translation = Vector3{dist(rng), dist(rng), dist(rng)},
      };
      transform.rotation = transform.rotation / triglav::sign(transform.rotation.w);

      const auto mat = transform.to_matrix();
      const auto decoded_transform = Transform3D::from_matrix(mat);

      EXPECT_TRUE(vec3_equals(decoded_transform.translation, transform.translation));
      EXPECT_TRUE(vec3_equals(decoded_transform.scale, transform.scale));
      EXPECT_TRUE(quat_equals(decoded_transform.rotation, transform.rotation));
   }
}

TEST(MathTest, LookAtTest)
{
   Matrix4x4 identity_look_at = Matrix4x4::look_at(Vector3{0, 0, 0}, Vector3{0, 0, -1}, Vector3{0, 1, 0});
   ASSERT_TRUE(mat4_equals(identity_look_at, Matrix4x4::identity()));
}

TEST(MathTest, PerspectiveTest)
{
   static constexpr Matrix4x4 expected_matrix = {
      {0.5f, 0, 0, 0},
      {0, -1.0f, 0, 0},
      {0, 0, -1.002f, -1},
      {0, 0, -0.2002f, 0},
   };

   const Matrix4x4 perspective = Matrix4x4::perspective_projection(triglav::PI / 2.0f, 2.0f, 0.1f, 100.0f);
   ASSERT_TRUE(mat4_equals(perspective, expected_matrix));
}

TEST(MathTest, OrthographicTest)
{
   static constexpr Matrix4x4 expected_matrix = {
      {0.4f, 0, 0, 0},
      {0, -0.222222f, 0, 0},
      {0, 0, -0.02002f, 0},
      {-0.2f, 0.111111f, -1.002f, 1.0f},
   };

   const Matrix4x4 ortho = Matrix4x4::orthographic_projection(-2.0f, 3.0f, -5.0f, 4.0f, 0.1f, 100.0f);
   ASSERT_TRUE(mat4_equals(ortho, expected_matrix));
}

TEST(MathTest, ClosestPoint)
{
   const auto result =
      triglav::find_closest_point_between_lines({0, 0, 0}, Vector3{1, 1, 0}.normalize(), {1, 0, 0}, Vector3{-1, 1, 0}.normalize());
   EXPECT_EQ(result, Vector3(0.5, 0.5, 0));
}

TEST(MathTest, ClosestPointToLine)
{
   const auto result = triglav::find_closest_point_on_line({0, 0, 0}, Vector3{1, 1, 0}.normalize(), Vector3{1, 2, 0});
   EXPECT_TRUE((result - Vector3(1.5, 1.5, 0)).dot(Vector3{1, 1, 1}) < 0.001f);
}

TEST(MathTest, QuaternionTest)
{
   static constexpr Vector3 forward = {1.0f, 0.0f, 0.0f};

   const auto quat = Quaternion::from_euler_angles(Vector3{PI / 2.0f, -PI / 4.0f, PI / 2.0f});
   const auto rotated = quat * forward;

   const auto recon_from_orient = Quaternion::from_oriented_vector(forward, rotated.normalize()).normalize();
   const auto also_rotated = (recon_from_orient * forward).normalize();

   EXPECT_TRUE(vec3_equals(rotated, also_rotated));

   const auto rot_matrix = Matrix3x3::rotation(quat);
   const auto recon_from_matrix = Quaternion::from_rotation_matrix(rot_matrix);
   EXPECT_TRUE(quat_equals(quat, recon_from_matrix));
}
