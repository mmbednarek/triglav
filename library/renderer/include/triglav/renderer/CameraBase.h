#pragma once

#include "triglav/geometry/Geometry.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace triglav::renderer {

class CameraBase
{
 public:
   virtual ~CameraBase() = default;

   void set_position(glm::vec3 position);
   void set_orientation(glm::quat orientation);
   void set_near_far_planes(float near, float far);
   void rotate(float pitch, float yaw);

   [[nodiscard]] glm::vec3 position() const;
   [[nodiscard]] glm::quat orientation() const;
   [[nodiscard]] float near_plane() const;
   [[nodiscard]] float far_plane() const;

   [[nodiscard]] const glm::mat4 &view_matrix() const;
   [[nodiscard]] const glm::mat4 &view_projection_matrix() const;
   [[nodiscard]] bool is_point_visible(glm::vec3 point) const;
   [[nodiscard]] bool is_bouding_box_visible(const geometry::BoundingBox &boudingBox,
                                             const glm::mat4 &modelMat) const;

   [[nodiscard]] virtual const glm::mat4 &projection_matrix() const = 0;
   [[nodiscard]] virtual float to_linear_depth(float depth) const   = 0;

 private:
   glm::vec3 m_position{};
   glm::quat m_orientation{0.0f, 0.0f, 0.0f, 1.0f};
   float m_nearPlane{0.1f};
   float m_farPlane{200.0f};

   mutable bool m_hasCachedViewMatrix{false};
   mutable glm::mat4 m_viewMat{};

 protected:
   mutable bool m_hasCachedViewProjectionMatrix{false};
   mutable glm::mat4 m_viewProjectionMat{};
};

}// namespace triglav::renderer