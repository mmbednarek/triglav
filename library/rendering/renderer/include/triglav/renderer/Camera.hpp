#pragma once

#include "CameraBase.hpp"

namespace triglav::renderer {

class Camera final : public CameraBase
{
 public:
   void set_viewport_size(float width, float height);

   [[nodiscard]] const glm::mat4& projection_matrix() const override;
   [[nodiscard]] float to_linear_depth(float depth) const override;
   [[nodiscard]] OrthoCameraProperties calculate_shadow_map(glm::quat light_orientation, float frustum_range, float back_extension) const;
   [[nodiscard]] geometry::Ray viewport_ray(Vector2 coord, float distance = 100.0f) const;

 private:
   float m_viewport_aspect{1.0f};
   float m_angle{0.7854f};

   mutable bool m_has_cached_projection_matrix{false};
   mutable glm::mat4 m_projection_mat{};
};

}// namespace triglav::renderer