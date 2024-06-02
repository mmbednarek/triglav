#include "CameraBase.h"

namespace triglav::renderer {

void CameraBase::set_position(const glm::vec3 position)
{
   if (position == m_position)
      return;

   m_hasCachedViewMatrix           = false;
   m_hasCachedViewProjectionMatrix = false;
   m_position                      = position;
}

void CameraBase::set_orientation(const glm::quat orientation)
{
   if (orientation == m_orientation)
      return;

   m_hasCachedViewMatrix           = false;
   m_hasCachedViewProjectionMatrix = false;
   m_orientation                   = orientation;
}

void CameraBase::set_near_far_planes(const float near, const float far)
{
   if (m_nearPlane == near && m_farPlane == far)
      return;

   m_hasCachedViewMatrix           = false;
   m_hasCachedViewProjectionMatrix = false;
   m_nearPlane                     = near;
   m_farPlane                      = far;
}

void CameraBase::rotate(const float pitch, const float yaw)
{
   m_hasCachedViewMatrix           = false;
   m_hasCachedViewProjectionMatrix = false;
   m_orientation                   = glm::quat{
           glm::vec3{pitch, 0.0f, yaw}
   };
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
   return m_nearPlane;
}

float CameraBase::far_plane() const
{
   return m_farPlane;
}

const glm::mat4 &CameraBase::view_projection_matrix() const
{
   if (not m_hasCachedViewProjectionMatrix) {
      m_viewProjectionMat             = this->projection_matrix() * this->view_matrix();
      m_hasCachedViewProjectionMatrix = true;
   }
   return m_viewProjectionMat;
}

bool CameraBase::is_point_visible(const glm::vec3 point) const
{
   const auto &mat = this->view_projection_matrix();
   auto pointVP    = mat * glm::vec4(point, 1.0f);
   pointVP /= pointVP.w;

   return (pointVP.x >= -1.0f) && (pointVP.x <= 1.0f) && (pointVP.y >= -1.0f) && (pointVP.y <= 1.0f);
}

bool CameraBase::is_bounding_box_visible(const geometry::BoundingBox &boudingBox,
                                        const glm::mat4 &modelMat) const
{
   const auto mat = this->view_projection_matrix() * modelMat;
   const std::array points{
           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.min.z},
           glm::vec3{boudingBox.min.x, boudingBox.min.y, boudingBox.max.z},
           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.min.z},
           glm::vec3{boudingBox.min.x, boudingBox.max.y, boudingBox.max.z},
           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.min.z},
           glm::vec3{boudingBox.max.x, boudingBox.min.y, boudingBox.max.z},
           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.min.z},
           glm::vec3{boudingBox.max.x, boudingBox.max.y, boudingBox.max.z},
   };

   glm::vec3 min{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
                 std::numeric_limits<float>::infinity()};
   glm::vec3 max{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
                 -std::numeric_limits<float>::infinity()};

   for (const auto point : points) {
      auto projectedPoint = mat * glm::vec4(point, 1.0);
      projectedPoint /= fabs(projectedPoint.w);
      const float linearZ = this->to_linear_depth(projectedPoint.z);

      if (projectedPoint.x < min.x) {
         min.x = projectedPoint.x;
      }
      if (projectedPoint.y < min.y) {
         min.y = projectedPoint.y;
      }
      if (linearZ < min.z) {
         min.z = linearZ;
      }
      if (projectedPoint.x > max.x) {
         max.x = projectedPoint.x;
      }
      if (projectedPoint.y > max.y) {
         max.y = projectedPoint.y;
      }
      if (linearZ > max.z) {
         max.z = linearZ;
      }
   }

   return min.x <= 1.0f && max.x >= -1.0f && min.y <= 1.0f && max.y >= -1.0f && min.z <= 1.0f && max.z >= 0.0f;
}

const glm::mat4 &CameraBase::view_matrix() const
{
   if (not m_hasCachedViewMatrix) {
      const auto lookVector = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
      const auto upVector   = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
      m_viewMat             = glm::lookAt(m_position, m_position + lookVector, upVector);
      m_hasCachedViewMatrix = true;
   }

   return m_viewMat;
}

}// namespace renderer