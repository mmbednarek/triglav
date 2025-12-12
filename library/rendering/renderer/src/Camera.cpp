#include "Camera.hpp"

#include "triglav/Debug.hpp"

namespace triglav::renderer {

void Camera::set_viewport_size(const float width, const float height)
{
   const auto aspect = width / height;
   if (aspect == m_viewport_aspect)
      return;

   m_has_cached_projection_matrix = false;
   m_has_cached_view_projection_matrix = false;
   m_viewport_aspect = aspect;
}

const glm::mat4& Camera::projection_matrix() const
{
   if (not m_has_cached_projection_matrix) {
      m_projection_mat = glm::perspective(m_angle, m_viewport_aspect, this->near_plane(), this->far_plane());
      m_has_cached_projection_matrix = true;
   }

   return m_projection_mat;
}

float Camera::to_linear_depth(const float depth) const
{
   return (2.0f * near_plane()) / (far_plane() + near_plane() - depth * (far_plane() - near_plane()));
}

OrthoCameraProperties Camera::calculate_shadow_map(const glm::quat light_orientation, const float frustum_range, float back_extension) const
{
   const auto right_vec = this->right_vector();
   const auto forward_vec = this->forward_vector();
   const auto up_vec = this->down_vector();

   const auto near_plane_point = this->position() + forward_vec * this->near_plane();
   const auto far_plane_point = this->position() + forward_vec * frustum_range;

   const auto near_height_half = std::tan(m_angle / 2) * this->near_plane();
   const auto near_width_half = near_height_half * m_viewport_aspect;

   const auto far_height_half = std::tan(m_angle / 2) * frustum_range;
   const auto far_width_half = far_height_half * m_viewport_aspect;

   std::array<glm::vec3, 8> frustum_points{};
   frustum_points[0] = near_plane_point + near_height_half * up_vec - near_width_half * right_vec;
   frustum_points[1] = near_plane_point + near_height_half * up_vec + near_width_half * right_vec;
   frustum_points[2] = near_plane_point - near_height_half * up_vec - near_width_half * right_vec;
   frustum_points[3] = near_plane_point - near_height_half * up_vec + near_width_half * right_vec;

   frustum_points[4] = far_plane_point + far_height_half * up_vec - far_width_half * right_vec;
   frustum_points[5] = far_plane_point + far_height_half * up_vec + far_width_half * right_vec;
   frustum_points[6] = far_plane_point - far_height_half * up_vec - far_width_half * right_vec;
   frustum_points[7] = far_plane_point - far_height_half * up_vec + far_width_half * right_vec;

   const auto light_base_x = light_orientation * glm::vec3{1.0f, 0.0f, 0.0f};
   const auto light_base_y = light_orientation * glm::vec3{0.0f, 1.0f, 0.0f};
   const auto light_base_z = light_orientation * glm::vec3{0.0f, 0.0f, 1.0f};

   // Change all the points to the new basis and find min/max
   glm::vec3 min{std::numeric_limits<float>::max()};
   glm::vec3 max{std::numeric_limits<float>::lowest()};

   const auto light_to_world_matrix = glm::mat3{light_base_x, light_base_y, light_base_z};
   const auto world_to_light_matrix = glm::inverse(light_to_world_matrix);

   for (auto& point_world : frustum_points) {
      const auto point_light_base = world_to_light_matrix * point_world;

      if (point_light_base.x < min.x)
         min.x = point_light_base.x;
      if (point_light_base.y < min.y)
         min.y = point_light_base.y;
      if (point_light_base.z < min.z)
         min.z = point_light_base.z;
      if (point_light_base.x > max.x)
         max.x = point_light_base.x;
      if (point_light_base.y > max.y)
         max.y = point_light_base.y;
      if (point_light_base.z > max.z)
         max.z = point_light_base.z;
   }

   auto range = max.y - min.y;
   glm::vec3 position{0.5f * (min.x + max.x), min.y - back_extension, 0.5f * (min.z + max.z)};

   const auto width = max.x - min.x;

   return OrthoCameraProperties{
      .position{light_to_world_matrix * position},
      .orientation = light_orientation,
      .range = back_extension + range,
      .aspect = width / (max.z - min.z),
      .view_space_width = width,
   };
}

geometry::Ray Camera::viewport_ray(const Vector2 coord, const float distance) const
{
   const auto near_place_mid = this->position() + this->forward_vector() * this->near_plane();
   const auto near_height_half = std::tan(m_angle / 2) * this->near_plane();
   const auto near_width_half = near_height_half * m_viewport_aspect;

   const auto origin = near_place_mid + coord.x * near_width_half * this->right_vector() + coord.y * near_height_half * this->down_vector();

   return {
      .origin = origin,
      .direction = normalize(origin - this->position()),
      .distance = distance,
   };
}

}// namespace triglav::renderer