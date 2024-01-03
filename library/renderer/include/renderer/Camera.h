#pragma once

#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

namespace renderer {

class Camera
{
 public:
   void set_position(glm::vec3 position);
   void set_orientation(glm::quat orientation);
   void set_viewport_size(float width, float height);

   [[nodiscard]] glm::vec3 position() const;

   [[nodiscard]] glm::mat4 matrix() const;
   [[nodiscard]] bool is_point_visible(glm::vec3 point) const;

 private:
   glm::vec3 m_position{};
   glm::quat m_orientation{};
   float m_nearPlane{0.1f};
   float m_farPlane{100.0f};
   float m_viewportAspect{1.0f};
   float m_angle{0.7854f};

   bool m_hasCachedMatrix{false};
   glm::mat4 m_viewProjMat{};
};

}// namespace renderer