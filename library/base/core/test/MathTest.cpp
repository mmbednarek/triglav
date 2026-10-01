#include "triglav/Math.hpp"
#include "triglav/testing_core/GTest.hpp"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <random>

using triglav::g_pi;
using triglav::Matrix3x3;
using triglav::Matrix4x4;
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

// [[nodiscard]] bool quat_equals(const Quaternion a, const Quaternion b)
// {
//    return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z) && float_equals(a.w, b.w);
// }

[[nodiscard]] bool vec3_equals(const Vector3 a, const Vector3 b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z);
}

[[nodiscard]] bool vec3_equals(const Vector3 a, const glm::vec3& b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z);
}

[[nodiscard]] bool vec4_equals(const Vector4 a, const glm::vec4& b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z) && float_equals(a.w, b.w);
}

[[nodiscard]] bool quat_equals(const Quaternion& a, const glm::quat& b)
{
   return float_equals(a.x, b.x) && float_equals(a.y, b.y) && float_equals(a.z, b.z) && float_equals(a.w, b.w);
}

[[nodiscard]] bool mat4_equals(const Matrix4x4& a, const glm::mat4& b)
{
   for (u32 c = 0; c < 4; ++c) {
      for (u32 r = 0; r < 4; ++r) {
         if (!float_equals(a[c][r], b[c][r]))
            return false;
      }
   }
   return true;
}

[[nodiscard]] bool mat3_equals(const Matrix3x3& a, const glm::mat3& b)
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
   glm::mat4 glm_mat = glm::mat4(1);
   ASSERT_TRUE(mat4_equals(mat, glm_mat));

   Vector4 vec{1, 2, 3, 4};
   ASSERT_EQ(mat * vec, vec);

   glm::vec4 glm_vec{1, 2, 3, 4};

   mat = Matrix4x4{
      {1, 2, 3, -4},
      {5, 8, 0, 4},
      {6, -9, 1, 5},
      {7, 0, 2, 5},
   };
   glm_mat = glm::mat4{
      // clang-format off
         1, 2, 3, -4,
         5, 8, 0, 4,
         6, -9, 1, 5,
         7, 0, 2, 5,
         // clang-format off
      };
   ASSERT_TRUE(mat4_equals(mat, glm_mat));
   ASSERT_TRUE(vec4_equals(mat * vec, glm_mat * glm_vec));

   Matrix4x4 inv_mat = mat.inverse();
   glm::mat4 glm_inv_mat = glm::inverse(glm_mat);
   ASSERT_TRUE(mat4_equals(inv_mat, glm_inv_mat));

   Matrix3x3 inv_mat3 = mat.shrink().inverse();
   glm::mat3 glm_inv_mat3 = glm::inverse(glm::mat3{glm_mat});
   ASSERT_TRUE(mat3_equals(inv_mat3, glm_inv_mat3));

   Matrix4x4 transposed_mat = mat.transpose();
   glm::mat4 glm_transposed_mat = glm::transpose(glm_mat);
   ASSERT_TRUE(mat4_equals(transposed_mat, glm_transposed_mat));

   Matrix4x4 trans_mat = Matrix4x4::translation({10, 20, -50});
   glm::mat4 glm_trans_mat = glm::translate(glm::mat4{1}, {10, 20, -50});
   ASSERT_TRUE(mat4_equals(trans_mat, glm_trans_mat));

   Matrix4x4 scale_mat = Matrix3x3::scale({2, 5, 9}).extend();
   glm::mat4 glm_scale_mat = glm::scale(glm::mat4{1}, {2, 5, 9});
   ASSERT_TRUE(mat4_equals(scale_mat, glm_scale_mat));

   Matrix4x4 rot_mat = Matrix3x3::rotation(Quaternion::from_euler_angles({triglav::PI / 4.0f, triglav::PI * 3.0f / 4.0f, triglav::PI * 3.0f / 2.0f})).extend();
   glm::mat4 glm_rot_mat = glm::mat4_cast(glm::quat(glm::vec3{triglav::PI / 4.0f, triglav::PI * 3.0f / 4.0f, triglav::PI * 3.0f / 2.0f}));
   ASSERT_TRUE(mat4_equals(rot_mat, glm_rot_mat));

   const auto quat = Quaternion::from_rotation_matrix(rot_mat.shrink());
   const auto glm_quat = glm::quat_cast(glm_rot_mat);
   ASSERT_TRUE(quat_equals(quat, glm_quat));

   Matrix4x4 mul_mat = trans_mat * rot_mat * scale_mat;
   glm::mat4 glm_mul_mat = glm_trans_mat * glm_rot_mat * glm_scale_mat;
   ASSERT_TRUE(mat4_equals(mul_mat, glm_mul_mat));
}

TEST(MathTest, BasicCase)
{
   std::mt19937 rng{1111};
   std::uniform_real_distribution<float> dist{-2, 2};

   for (auto i = 0; i < 100; ++i) {
      Transform3D transform{
         .rotation = Quaternion::from_euler_angles({dist(rng) * g_pi, dist(rng) * g_pi, dist(rng) * g_pi}).normalize(),
         .scale = Vector3{0.1f + abs(dist(rng)), 0.1f + abs(dist(rng)), 0.1f + abs(dist(rng))},
         .translation = Vector3{dist(rng), dist(rng), dist(rng)},
      };
      transform.rotation = transform.rotation / triglav::sign(transform.rotation.w);

      auto glm_translation = glm::vec3{transform.translation.x, transform.translation.y, transform.translation.z};
      EXPECT_TRUE(mat4_equals(Matrix4x4::translation(transform.translation), glm::translate(glm::mat4(1.0f), glm_translation)));

      auto glm_scale = glm::vec3{transform.scale.x, transform.scale.y, transform.scale.z};
      EXPECT_TRUE(mat4_equals(Matrix3x3::scale(transform.scale).extend(), glm::scale(glm::mat4(1.0f), glm_scale)));

      auto glm_rotation = glm::quat{transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z};
      EXPECT_TRUE(mat4_equals(Matrix3x3::rotation(transform.rotation).extend(), glm::mat4_cast(glm_rotation)));

      const auto glm_mat = glm::translate(glm::mat4(1.0f), glm_translation) * glm::mat4_cast(glm_rotation) * glm::scale(glm::mat4(1.0f), glm_scale);

      const auto mat = transform.to_matrix();
      EXPECT_TRUE(mat4_equals(mat, glm_mat));

      const auto decoded_transform = Transform3D::from_matrix(mat);

      const auto translation = glm::vec4(glm_mat[3]);
      EXPECT_TRUE(vec4_equals(Vector4{decoded_transform.translation, 1.0f}, translation));

      const auto glm_vec1 = glm::vec3(glm_mat[0]);
      const auto glm_vec2 = glm::vec3(glm_mat[1]);
      const auto glm_vec3 = glm::vec3(glm_mat[2]);

      const glm::vec3 glm_decoded_scale(glm::length(glm_vec1), glm::length(glm_vec2), glm::length(glm_vec3));
      ASSERT_TRUE(vec3_equals(decoded_transform.scale, glm_decoded_scale));

      // auto decoded_matrix = decoded_transform.to_matrix();
      // ASSERT_TRUE(mat4_equals(decoded_matrix, glm_mat));


      // const glm::mat3 glm_rot_mat{glm_vec1 / glm_decoded_scale.x, glm_vec2 / glm_decoded_scale.y, glm_vec3 / glm_decoded_scale.z};
      //
      // auto glm_decoded_rotation{glm::normalize(glm::quat_cast(glm_rot_mat))};
      // glm_decoded_rotation /= triglav::sign(glm_decoded_rotation.w);

      // EXPECT_TRUE(quat_equals(decoded_transform.rotation, transform.rotation));
      // EXPECT_TRUE(vec3_equals(decoded_transform.scale, transform.scale));
      // EXPECT_TRUE(vec3_equals(decoded_transform.translation, transform.translation));
   }
}

TEST(MathTest, LookAtTest)
{
   Matrix4x4 look_at = Matrix4x4::look_at(Vector3{1, 1, 1}, Vector3{0, 0, 0}, Vector3{0, 1, 0});
   glm::mat4 glm_look_at = glm::lookAt(glm::vec3{1, 1, 1}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
   ASSERT_TRUE(mat4_equals(look_at, glm_look_at));
}

TEST(MathTest, PerspectiveTest)
{
   Matrix4x4 perspective = Matrix4x4::perspective_projection(triglav::PI / 2.0f, 2.0f, 0.1f, 100.0f);
   glm::mat4 glm_perspective = glm::perspective(triglav::PI / 2.0f, 2.0f, 0.1f, 100.0f);
   ASSERT_TRUE(mat4_equals(perspective, glm_perspective));
}

TEST(MathTest, OrthographicTest)
{
   Matrix4x4 ortho = Matrix4x4::orthographic_projection(-2.0f, 3.0f, -5.0f, 4.0f, 0.1f, 100.0f);
   glm::mat4 glm_ortho = glm::ortho(-2.0f, 3.0f, -5.0f, 4.0f, 0.1f, 100.0f);
   ASSERT_TRUE(mat4_equals(ortho, glm_ortho));
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
   const auto forward = Vector3{1.0f, 0.0f, 0.0f};

   const auto quat = Quaternion::from_euler_angles(Vector3{triglav::PI / 2.0f, -triglav::PI / 4.0f, triglav::PI / 2.0f});
   const auto rotated = quat * forward;

   const auto glm_quat = glm::quat(glm::vec3{triglav::PI / 2.0f, -triglav::PI / 4.0f, triglav::PI / 2.0f});
   ASSERT_TRUE(quat_equals(quat, glm_quat));

   const auto glm_rotated = glm_quat * glm::vec3{1.0f, 0.0f, 0.0f};
   ASSERT_TRUE(vec3_equals(rotated, glm_rotated));

   const auto recon = Quaternion::from_oriented_vector(forward, rotated.normalize());
   const auto glm_recon = glm::rotation(glm::vec3{1.0f, 0.0f, 0.0f}, glm::normalize(glm_rotated));
   ASSERT_TRUE(quat_equals(recon, glm_recon));

   const auto also_rotated = (recon * forward).normalize();

   EXPECT_TRUE(vec3_equals(rotated, also_rotated));
}
