#include "CameraBase.hpp"

namespace triglav::renderer {

void CameraBase::set_position(const glm::vec3 position)
{
   if (position == m_position)
      return;

   m_has_cached_view_matrix = false;
   m_has_cached_view_projection_matrix = false;
   m_position = position;
}

void CameraBase::set_orientation(const glm::quat orientation)
{
   if (orientation == m_orientation)
      return;

   m_has_cached_view_matrix = false;
   m_has_cached_view_projection_matrix = false;
   m_orientation = orientation;
}

void CameraBase::set_near_far_planes(const float near, const float far)
{
   if (m_near_plane == near && m_far_plane == far)
      return;

   m_has_cached_view_matrix = false;
   m_has_cached_view_projection_matrix = false;
   m_near_plane = near;
   m_far_plane = far;
}

void CameraBase::rotate(const float pitch, const float yaw)
{
   m_has_cached_view_matrix = false;
   m_has_cached_view_projection_matrix = false;
   m_orientation = glm::quat{glm::vec3{pitch, 0.0f, yaw}};
}

glm::vec3 CameraBase::position() const
{
   return m_position;
}

glm::quat CameraBase::orientation() const
{
   return m_orientation;
}

float CameraBase::near_plane() const
{
   return m_near_plane;
}

float CameraBase::far_plane() const
{
   return m_far_plane;
}

const glm::mat4& CameraBase::view_projection_matrix() const
{
   if (not m_has_cached_view_projection_matrix) {
      m_view_projection_mat = this->projection_matrix() * this->view_matrix();
      m_has_cached_view_projection_matrix = true;
   }
   return m_view_projection_mat;
}

bool CameraBase::is_point_visible(const glm::vec3 point) const
{
   const auto& mat = this->view_projection_matrix();
   auto point_vp = mat * glm::vec4(point, 1.0f);
   point_vp /= point_vp.w;

   return (point_vp.x >= -1.0f) && (point_vp.x <= 1.0f) && (point_vp.y >= -1.0f) && (point_vp.y <= 1.0f);
}

bool CameraBase::is_bounding_box_visible(const geometry::BoundingBox& bounding_box, const glm::mat4& model_mat) const
{
   const auto mat = this->view_projection_matrix() * model_mat;
   const std::array points{
      glm::vec3{bounding_box.min.x, bounding_box.min.y, bounding_box.min.z},
      glm::vec3{bounding_box.min.x, bounding_box.min.y, bounding_box.max.z},
      glm::vec3{bounding_box.min.x, bounding_box.max.y, bounding_box.min.z},
      glm::vec3{bounding_box.min.x, bounding_box.max.y, bounding_box.max.z},
      glm::vec3{bounding_box.max.x, bounding_box.min.y, bounding_box.min.z},
      glm::vec3{bounding_box.max.x, bounding_box.min.y, bounding_box.max.z},
      glm::vec3{bounding_box.max.x, bounding_box.max.y, bounding_box.min.z},
      glm::vec3{bounding_box.max.x, bounding_box.max.y, bounding_box.max.z},
   };

   glm::vec3 min{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
   glm::vec3 max{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};

   for (const auto point : points) {
      auto projected_point = mat * glm::vec4(point, 1.0);

      // linear Z from projected point.
      const float linear_z = this->to_linear_depth(projected_point.z / projected_point.w);

      // divide by abs value of the w term to avoid reflecting the point by the origin.
      auto hm_point = projected_point;
      hm_point /= fabs(projected_point.w);

      if (hm_point.x < min.x) {
         min.x = hm_point.x;
      }
      if (hm_point.y < min.y) {
         min.y = hm_point.y;
      }
      if (linear_z < min.z) {
         min.z = linear_z;
      }
      if (hm_point.x > max.x) {
         max.x = hm_point.x;
      }
      if (hm_point.y > max.y) {
         max.y = hm_point.y;
      }
      if (linear_z > max.z) {
         max.z = linear_z;
      }
   }

   return min.x <= 1.0f && max.x >= -1.0f && min.y <= 1.0f && max.y >= -1.0f && min.z <= 1.0f && max.z >= 0.0f;
}

Vector3 CameraBase::forward_vector() const
{
   return m_orientation * Vector3{0, 1, 0};
}

Vector3 CameraBase::right_vector() const
{
   return m_orientation * Vector3{1, 0, 0};
}

Vector3 CameraBase::down_vector() const
{
   return m_orientation * Vector3{0, 0, 1};
}

const glm::mat4& CameraBase::view_matrix() const
{
   if (not m_has_cached_view_matrix) {
      const auto look_vector = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
      const auto up_vector = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
      m_view_mat = glm::lookAt(m_position, m_position + look_vector, up_vector);
      m_has_cached_view_matrix = true;
   }

   return m_view_mat;
}

}// namespace triglav::renderer