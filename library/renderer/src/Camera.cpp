#include "Camera.h"

namespace renderer {

void Camera::set_position(const glm::vec3 position)
{
   if (position == m_position)
      return;

   m_hasCachedViewMatrix           = false;
   m_hasCachedViewProjectionMatrix = false;
   m_position                      = position;
}

void Camera::set_orientation(const glm::quat orientation)
{
   if (orientation == m_orientation)
      return;

   m_hasCachedViewMatrix           = false;
   m_hasCachedViewProjectionMatrix = false;
   m_orientation                   = orientation;
}

void Camera::set_viewport_size(const float width, const float height)
{
   const auto aspect = width / height;
   if (aspect == m_viewportAspect)
      return;

   m_hasCachedProjectionMatrix     = false;
   m_hasCachedViewProjectionMatrix = false;
   m_viewportAspect                = aspect;
}

void Camera::rotate(const float pitch, const float yaw)
{
   m_hasCachedViewMatrix           = false;
   m_hasCachedViewProjectionMatrix = false;
   m_orientation                   = glm::quat{
           glm::vec3{pitch, 0.0f, yaw}
   };
}

glm::vec3 Camera::position() const
{
   return m_position;
}

glm::quat Camera::orientation() const
{
   return m_orientation;
}

bool Camera::is_point_visible(const glm::vec3 point) const
{
   const auto &mat = this->view_proj_matrix();
   auto pointVP    = mat * glm::vec4(point, 1.0f);
   pointVP /= pointVP.w;

   return (pointVP.x >= -1.0f) && (pointVP.x <= 1.0f) && (pointVP.y >= -1.0f) && (pointVP.y <= 1.0f);
}

bool Camera::is_bouding_box_visible(const geometry::BoundingBox &boudingBox, const glm::mat4 &modelMat) const
{
   const auto mat = this->view_proj_matrix() * modelMat;
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
      projectedPoint /= projectedPoint.w;
      const float linearZ = (2.0f * m_nearPlane) /
                            (m_farPlane + m_nearPlane - projectedPoint.z * (m_farPlane - m_nearPlane));

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

   return min.x <= 1.0f && max.x >= -1.0f && min.y <= 1.0f && max.y >= -1.0f && min.z <= 1.0f &&
          max.z >= 0.0f;
}

const glm::mat4 &Camera::view_proj_matrix() const
{
   if (not m_hasCachedViewProjectionMatrix) {
      m_viewProjectionMat             = this->projection_matrix() * this->view_matrix();
      m_hasCachedViewProjectionMatrix = true;
   }
   return m_viewProjectionMat;
}

const glm::mat4 &Camera::view_matrix() const
{
   if (not m_hasCachedViewMatrix) {
      const auto lookVector = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
      const auto upVector   = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
      m_viewMat             = glm::lookAt(m_position, m_position + lookVector, upVector);
      m_hasCachedViewMatrix = true;
   }

   return m_viewMat;
}

const glm::mat4 &Camera::projection_matrix() const
{
   if (not m_hasCachedProjectionMatrix) {
      m_projectionMat             = glm::perspective(m_angle, m_viewportAspect, m_nearPlane, m_farPlane);
      m_hasCachedProjectionMatrix = true;
   }

   return m_projectionMat;
}

}// namespace renderer