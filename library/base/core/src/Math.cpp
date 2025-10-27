#include "Math.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace triglav {

Transform3D Transform3D::identity()
{
   return Transform3D{.rotation = {1, 0, 0, 0}, .scale = {1, 1, 1}, .translation = {0, 0, 0}};
}

Transform3D Transform3D::from_matrix(const Matrix4x4& matrix)
{
   const auto translation = Vector4(matrix[3]);

   const auto vec1 = Vector3(matrix[0]);
   const auto vec2 = Vector3(matrix[1]);
   const auto vec3 = Vector3(matrix[2]);

   const Vector3 scale(glm::length(vec1), glm::length(vec2), glm::length(vec3));

   const glm::mat3 rotMat{vec1 / scale.x, vec2 / scale.y, vec3 / scale.z};

   const auto rotation{glm::normalize(glm::quat_cast(rotMat))};
   return Transform3D{.rotation = rotation / sign(rotation.w), .scale = scale, .translation = translation};
}

Matrix4x4 Transform3D::to_matrix() const
{
   return glm::translate(glm::mat4(1.0f), this->translation) * glm::mat4_cast(this->rotation) * glm::scale(glm::mat4(1.0f), this->scale);
}

}// namespace triglav