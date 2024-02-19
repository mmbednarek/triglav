#include "Camera.h"

namespace triglav::renderer {

void Camera::set_viewport_size(const float width, const float height)
{
   const auto aspect = width / height;
   if (aspect == m_viewportAspect)
      return;

   m_hasCachedProjectionMatrix     = false;
   m_hasCachedViewProjectionMatrix = false;
   m_viewportAspect                = aspect;
}

const glm::mat4 &Camera::projection_matrix() const
{
   if (not m_hasCachedProjectionMatrix) {
      m_projectionMat = glm::perspective(m_angle, m_viewportAspect, this->near_plane(), this->far_plane());
      m_hasCachedProjectionMatrix = true;
   }

   return m_projectionMat;
}

float Camera::to_linear_depth(const float depth) const
{
   return (2.0f * near_plane()) / (far_plane() + near_plane() - depth * (far_plane() - near_plane()));
}

}// namespace renderer