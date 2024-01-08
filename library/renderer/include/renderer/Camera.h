#pragma once

#include <geometry/Geometry.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

namespace renderer {

class Camera
{
 public:
   void set_position(glm::vec3 position);
   void set_orientation(glm::quat orientation);
   void set_viewport_size(float width, float height);
   void rotate(float pitch, float yaw);

   [[nodiscard]] glm::vec3 position() const;
   [[nodiscard]] glm::quat orientation() const;

   [[nodiscard]] const glm::mat4 &matrix() const;
   [[nodiscard]] bool is_point_visible(glm::vec3 point) const;
   [[nodiscard]] bool is_bouding_box_visible(const geometry::BoundingBox &boudingBox, const glm::mat4 &modelMat) const;

 private:
   glm::vec3 m_position{};
   glm::quat m_orientation{0.0f, 0.0f, 0.0f, 1.0f};
   float m_nearPlane{0.1f};
   float m_farPlane{200.0f};
   float m_viewportAspect{1.0f};
   float m_angle{0.7854f};

   mutable bool m_hasCachedMatrix{false};
   mutable glm::mat4 m_viewProjMat{};
};

}// namespace renderer