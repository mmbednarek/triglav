#include "OrthoCamera.hpp"

namespace triglav::renderer {

void OrthoCamera::set_viewport_size(const float width, const float height)
{
   this->set_aspect(width / height);
}

void OrthoCamera::set_aspect(const float aspect)
{
   if (aspect == m_aspect)
      return;

   m_hasCachedProjectionMatrix = false;
   m_hasCachedViewProjectionMatrix = false;
   m_aspect = aspect;
}

void OrthoCamera::set_viewspace_width(const float width)
{
   if (m_viewSpaceWidth == width)
      return;

   m_hasCachedProjectionMatrix = false;
   m_hasCachedViewProjectionMatrix = false;
   m_viewSpaceWidth = width;
}

const glm::mat4& OrthoCamera::projection_matrix() const
{
   if (not m_hasCachedProjectionMatrix) {
      const auto halfWidth = 0.5f * m_viewSpaceWidth;
      const auto halfHeight = 0.5f * m_viewSpaceWidth / m_aspect;

      m_projectionMat = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, this->near_plane(), this->far_plane());
      //      m_projectionMat = glm::perspective(static_cast<float>(geometry::g_pi / 2), m_aspect, 0.01f, this->far_plane());
      m_hasCachedProjectionMatrix = true;
   }

   return m_projectionMat;
}

float OrthoCamera::to_linear_depth(const float depth) const
{
   return depth;
}

OrthoCamera OrthoCamera::from_properties(const OrthoCameraProperties& props)
{
   OrthoCamera result;
   result.set_position(props.position);
   result.set_aspect(props.aspect);
   result.set_viewspace_width(props.viewSpaceWidth);
   result.set_orientation(props.orientation);
   result.set_near_far_planes(0, props.range);

   return result;
}

}// namespace triglav::renderer
