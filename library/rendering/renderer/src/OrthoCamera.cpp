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

   m_has_cached_projection_matrix = false;
   m_has_cached_view_projection_matrix = false;
   m_aspect = aspect;
}

void OrthoCamera::set_viewspace_width(const float width)
{
   if (m_view_space_width == width)
      return;

   m_has_cached_projection_matrix = false;
   m_has_cached_view_projection_matrix = false;
   m_view_space_width = width;
}

const glm::mat4& OrthoCamera::projection_matrix() const
{
   if (not m_has_cached_projection_matrix) {
      const auto half_width = 0.5f * m_view_space_width;
      const auto half_height = 0.5f * m_view_space_width / m_aspect;

      m_projection_mat = glm::ortho(-half_width, half_width, -half_height, half_height, this->near_plane(), this->far_plane());
      //      m_projection_mat = glm::perspective(static_cast<float>(geometry::g_pi / 2), m_aspect, 0.01f, this->far_plane());
      m_has_cached_projection_matrix = true;
   }

   return m_projection_mat;
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
   result.set_viewspace_width(props.view_space_width);
   result.set_orientation(props.orientation);
   result.set_near_far_planes(0, props.range);

   return result;
}

}// namespace triglav::renderer
