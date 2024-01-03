#include "Camera.h"

#include <chrono>
#include <iostream>

namespace renderer {

void Camera::set_position(const glm::vec3 position)
{
   if (position == m_position)
      return;

   m_hasCachedMatrix = false;
   m_position        = position;
}

void Camera::set_orientation(const glm::quat orientation)
{
   if (orientation == m_orientation)
      return;

   m_hasCachedMatrix = false;
   m_orientation     = orientation;
}

void Camera::set_viewport_size(const float width, const float height)
{
   const auto aspect = width / height;
   if (aspect == m_viewportAspect)
      return;

   m_hasCachedMatrix = false;
   m_viewportAspect  = aspect;
}

glm::vec3 Camera::position() const
{
   return m_position;
}

bool Camera::is_point_visible(const glm::vec3 point) const
{
   const auto mat     = this->matrix();
   const auto pointVP = mat * glm::vec4(point, 1.0f);

   return pointVP.x >= -1.0f && pointVP.x <= 1.0f && pointVP.y >= -1.0f && pointVP.y <= 1.0f;
}

glm::mat4 Camera::matrix() const
{
   // if (not m_hasCachedMatrix) {
   //    const auto lookVector = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
   //    const auto upVector   = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);
   //
   //    const auto view       = glm::lookAt(m_position, m_position + lookVector, upVector);
   //    const auto projection = glm::perspective(m_angle, m_viewportAspect, m_nearPlane, m_farPlane);
   //
   //    m_viewProjMat     = projection * view;
   //    m_hasCachedMatrix = true;
   // }
   //
   // return m_viewProjMat;

   const auto lookVector = m_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
   const auto upVector   = m_orientation * glm::vec3(0.0f, 0.0f, 1.0f);

   const auto view       = glm::lookAt(m_position, m_position + lookVector, upVector);
   const auto projection = glm::perspective(m_angle, m_viewportAspect, m_nearPlane, m_farPlane);
   return projection * view;
}

}// namespace renderer