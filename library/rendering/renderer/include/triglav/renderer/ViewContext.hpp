#pragma once

#include "Camera.hpp"

#include "triglav/Event.hpp"
#include "triglav/graphics_api/GraphicsApi.hpp"

namespace triglav::renderer {

class ViewContext
{
 public:
   TG_EVENT(OnViewportChange, const graphics_api::Resolution&)
   TG_EVENT(OnViewUpdated, const Camera&)

   void update_resolution(const graphics_api::Resolution& resolution);
   void set_camera(Vector3 position, Quaternion orientation);
   void send_view_changed() const;
   void update_orientation(float delta_yaw, float delta_pitch);

   [[nodiscard]] const Camera& camera() const;
   [[nodiscard]] Camera& camera();

   [[nodiscard]] float yaw() const;
   [[nodiscard]] float pitch() const;

 private:
   float m_yaw{4.42f};
   float m_pitch{-0.6f};
   Camera m_camera{};
};

}// namespace triglav::renderer
