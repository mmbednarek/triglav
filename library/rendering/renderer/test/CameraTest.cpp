#include "triglav/testing_core/GTest.hpp"

#include "triglav/renderer/Camera.hpp"

using triglav::Matrix3x3;
using triglav::Quaternion;
using triglav::Vector2;
using triglav::Vector3;
using triglav::Vector4;
using triglav::geometry::Ray;
using triglav::renderer::Camera;

constexpr float TOLERANCE = 0.01f;

constexpr bool compare(const Vector3& a, const Vector3& b)
{
   float err = 0.0f;
   err += 0.333f * std::abs(a.x - b.x);
   err += 0.333f * std::abs(a.y - b.y);
   err += 0.333f * std::abs(a.z - b.z);
   return err < TOLERANCE;
}

constexpr bool compare(const Vector4& a, const Vector4& b)
{
   float err = 0.0f;
   err += 0.333f * std::abs(a.x - b.x);
   err += 0.333f * std::abs(a.y - b.y);
   err += 0.333f * std::abs(a.z - b.z);
   err += 0.333f * std::abs(a.w - b.w);
   return err < TOLERANCE;
}

struct CameraTestParams
{
   Vector3 position;
   Vector2 viewport_size;
   Quaternion orientation;
   float near, far;
   Vector2 screen_pos;
};

class CameraTest : public ::testing::TestWithParam<CameraTestParams>
{};

INSTANTIATE_TEST_SUITE_P(DepthTests, CameraTest,
                         ::testing::Values(
                            CameraTestParams{
                               .position = {0, 0, 0},
                               .viewport_size = {100, 100},
                               .orientation = {1, 0, 0, 0},
                               .near = 1.0f,
                               .far = 100.0f,
                               .screen_pos = {0, 0},
                            },
                            CameraTestParams{
                               .position = {-10, 20, 11},
                               .viewport_size = {100, 200},
                               .orientation = {0.854, 0.146, 0.354, 0.354},
                               .near = 1.0f,
                               .far = 100.0f,
                               .screen_pos = {0.5f, 0.5f},
                            },
                            CameraTestParams{
                               .position = {10, 0, 2.5},
                               .viewport_size = {300, 50},
                               .orientation = {0.379, -0.597, 0.384, -0.593},
                               .near = 0.5f,
                               .far = 200.0f,
                               .screen_pos = {-0.25f, 1.0f},
                            }));

TEST_P(CameraTest, DepthTests)
{
   const auto params = GetParam();

   Camera camera;
   camera.set_position(params.position);
   camera.set_viewport_size(params.viewport_size.x, params.viewport_size.y);
   camera.set_orientation(params.orientation);
   camera.set_near_far_planes(params.near, params.far);

   Ray ray = camera.viewport_ray(params.screen_pos);

   static constexpr float DISTANCE = 10.0f;
   auto world_pos = ray.origin + ray.direction * DISTANCE;

   const auto view = camera.view_matrix();
   const auto view_inv = glm::inverse(view);

   const auto view_pos = view * Vector4{world_pos, 1.0f};

   const auto dir_view = camera.view_space_dir(params.screen_pos);
   ASSERT_TRUE(compare(dir_view, Matrix3x3{view} * ray.direction));

   const auto view_proj = camera.view_projection_matrix();

   const auto pers_pos = view_proj * Vector4{world_pos, 1.0f};

   float depth_val = pers_pos.z / pers_pos.w;
   float linear_distance = camera.to_linear_depth(depth_val);

   const auto near_height_half = std::tan(camera.angle() / 2) * camera.near_plane();
   const auto near_width_half = near_height_half * camera.viewport_aspect();

   float depth_ratio = linear_distance / camera.near_plane();

   Vector4 reconstructed_view{params.screen_pos.x * near_width_half * depth_ratio, params.screen_pos.y * near_height_half * depth_ratio,
                              -linear_distance, 1.0f};
   EXPECT_TRUE(compare(reconstructed_view, view_pos));
   Vector4 reconstructed_world_pos = view_inv * reconstructed_view;
   EXPECT_TRUE(compare(Vector3{reconstructed_world_pos}, world_pos));
}
