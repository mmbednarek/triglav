#include "Camera.h"

#include "triglav/Debug.hpp"

namespace triglav::renderer {

void Camera::set_viewport_size(const float width, const float height)
{
   const auto aspect = width / height;
   if (aspect == m_viewportAspect)
      return;

   m_hasCachedProjectionMatrix = false;
   m_hasCachedViewProjectionMatrix = false;
   m_viewportAspect = aspect;
}

const glm::mat4& Camera::projection_matrix() const
{
   if (not m_hasCachedProjectionMatrix) {
      m_projectionMat = glm::perspective(m_angle, m_viewportAspect, this->near_plane(), this->far_plane());
      m_hasCachedProjectionMatrix = true;
   }

   return m_projectionMat;
}

float Camera::to_linear_depth(const float depth) const
{
   return (2.0f * near_plane()) / (far_plane() + near_plane() - depth * (far_plane() - near_plane()));
}

OrthoCameraProperties Camera::calculate_shadow_map(const glm::quat lightOrientation, const float frustumRange, float backExtension) const
{
   const auto rightVec = this->orientation() * glm::vec3(1.0f, 0.0f, 0.0f);
   const auto forwardVec = this->orientation() * glm::vec3(0.0f, 1.0f, 0.0f);
   const auto upVec = this->orientation() * glm::vec3(0.0f, 0.0f, 1.0f);

   const auto nearPlanePoint = this->position() + forwardVec * this->near_plane();
   const auto farPlanePoint = this->position() + forwardVec * frustumRange;

   const auto nearHeightHalf = std::tan(m_angle / 2) * this->near_plane();
   const auto nearWidthHalf = nearHeightHalf * m_viewportAspect;

   const auto farHeightHalf = std::tan(m_angle / 2) * frustumRange;
   const auto farWidthHalf = farHeightHalf * m_viewportAspect;

   std::array<glm::vec3, 8> frustumPoints{};
   frustumPoints[0] = nearPlanePoint + nearHeightHalf * upVec - nearWidthHalf * rightVec;
   frustumPoints[1] = nearPlanePoint + nearHeightHalf * upVec + nearWidthHalf * rightVec;
   frustumPoints[2] = nearPlanePoint - nearHeightHalf * upVec - nearWidthHalf * rightVec;
   frustumPoints[3] = nearPlanePoint - nearHeightHalf * upVec + nearWidthHalf * rightVec;

   frustumPoints[4] = farPlanePoint + farHeightHalf * upVec - farWidthHalf * rightVec;
   frustumPoints[5] = farPlanePoint + farHeightHalf * upVec + farWidthHalf * rightVec;
   frustumPoints[6] = farPlanePoint - farHeightHalf * upVec - farWidthHalf * rightVec;
   frustumPoints[7] = farPlanePoint - farHeightHalf * upVec + farWidthHalf * rightVec;

   const auto lightBaseX = lightOrientation * glm::vec3{1.0f, 0.0f, 0.0f};
   const auto lightBaseY = lightOrientation * glm::vec3{0.0f, 1.0f, 0.0f};
   const auto lightBaseZ = lightOrientation * glm::vec3{0.0f, 0.0f, 1.0f};

   // Change all the points to the new basis and find min/max
   glm::vec3 min{std::numeric_limits<float>::max()};
   glm::vec3 max{std::numeric_limits<float>::lowest()};

   const auto lightToWorldMatrix = glm::mat3{lightBaseX, lightBaseY, lightBaseZ};
   const auto worldToLightMatrix = glm::inverse(lightToWorldMatrix);

   for (auto& pointWorld : frustumPoints) {
      const auto pointLightBase = worldToLightMatrix * pointWorld;

      if (pointLightBase.x < min.x)
         min.x = pointLightBase.x;
      if (pointLightBase.y < min.y)
         min.y = pointLightBase.y;
      if (pointLightBase.z < min.z)
         min.z = pointLightBase.z;
      if (pointLightBase.x > max.x)
         max.x = pointLightBase.x;
      if (pointLightBase.y > max.y)
         max.y = pointLightBase.y;
      if (pointLightBase.z > max.z)
         max.z = pointLightBase.z;
   }

   auto range = max.y - min.y;
   glm::vec3 position{0.5f * (min.x + max.x), min.y - backExtension, 0.5f * (min.z + max.z)};

   const auto width = max.x - min.x;

   return OrthoCameraProperties{
      .position{lightToWorldMatrix * position},
      .orientation = lightOrientation,
      .range = backExtension + range,
      .aspect = width / (max.z - min.z),
      .viewSpaceWidth = width,
   };
}

}// namespace triglav::renderer