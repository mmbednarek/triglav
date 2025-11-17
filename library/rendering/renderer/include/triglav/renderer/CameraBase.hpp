#pragma once

#include "triglav/geometry/Geometry.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace triglav::renderer {

struct OrthoCameraProperties
{
   glm::vec3 position;
   glm::quat orientation;
   float range;
   float aspect;
   float view_space_width;
};

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

   [[nodiscard]] const glm::mat4& view_matrix() const;
   [[nodiscard]] const glm::mat4& view_projection_matrix() const;
   [[nodiscard]] bool is_point_visible(glm::vec3 point) const;
   [[nodiscard]] bool is_bounding_box_visible(const geometry::BoundingBox& bounding_box, const glm::mat4& model_mat) const;
   [[nodiscard]] Vector3 forward_vector() const;
   [[nodiscard]] Vector3 right_vector() const;
   [[nodiscard]] Vector3 down_vector() const;

   [[nodiscard]] virtual const glm::mat4& projection_matrix() const = 0;
   [[nodiscard]] virtual float to_linear_depth(float depth) const = 0;

 private:
   glm::vec3 m_position{};
   glm::quat m_orientation{0.0f, 0.0f, 0.0f, 1.0f};
   float m_near_plane{0.1f};
   float m_far_plane{200.0f};

   mutable bool m_has_cached_view_matrix{false};
   mutable glm::mat4 m_view_mat{};

 protected:
   mutable bool m_has_cached_view_projection_matrix{false};
   mutable glm::mat4 m_view_projection_mat{};
};

}// namespace triglav::renderer