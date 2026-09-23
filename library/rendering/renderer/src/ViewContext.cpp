#include "ViewContext.hpp"

namespace triglav::renderer {

void ViewContext::update_resolution(const graphics_api::Resolution& resolution)
{
   const auto [width, height] = resolution;
   m_camera.set_viewport_size(static_cast<float>(width), static_cast<float>(height));

   event_OnViewportChange.publish(resolution);
   this->send_view_changed();
}

void ViewContext::set_camera(const Vector3 position, const Quaternion orientation)
{
   m_camera.set_position(position);
   m_camera.set_orientation(orientation);
   event_OnViewUpdated.publish(m_camera);
}

void ViewContext::send_view_changed() const
{
   event_OnViewUpdated.publish(m_camera);
}

void ViewContext::update_orientation(const float delta_yaw, const float delta_pitch)
{
   m_yaw += delta_yaw;
   while (m_yaw < 0) {
      m_yaw += 2 * MATH_PI;
   }
   while (m_yaw >= 2 * MATH_PI) {
      m_yaw -= 2 * MATH_PI;
   }

   m_pitch += delta_pitch;
   m_pitch = std::clamp(m_pitch, -static_cast<float>(MATH_PI) / 2.0f + 0.01f, static_cast<float>(MATH_PI) / 2.0f - 0.01f);

   this->camera().set_orientation(glm::quat{glm::vec3{m_pitch, 0.0f, m_yaw}});
   this->send_view_changed();
}

const Camera& ViewContext::camera() const
{
   return m_camera;
}

Camera& ViewContext::camera()
{
   return m_camera;
}

float ViewContext::yaw() const
{
   return m_yaw;
}

float ViewContext::pitch() const
{
   return m_pitch;
}

}// namespace triglav::renderer
