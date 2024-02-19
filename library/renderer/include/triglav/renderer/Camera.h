#pragma once

#include "CameraBase.h"

namespace triglav::renderer {

class Camera final : public CameraBase
{
 public:
   void set_viewport_size(float width, float height);

   [[nodiscard]] const glm::mat4 &projection_matrix() const override;
   [[nodiscard]] float to_linear_depth(float depth) const override;

 private:
   float m_viewportAspect{1.0f};
   float m_angle{0.7854f};

   mutable bool m_hasCachedProjectionMatrix{false};
   mutable glm::mat4 m_projectionMat{};
};

}// namespace renderer