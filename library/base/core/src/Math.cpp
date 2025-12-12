#include "Math.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

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

   const glm::mat3 rot_mat{vec1 / scale.x, vec2 / scale.y, vec3 / scale.z};

   const auto rotation{glm::normalize(glm::quat_cast(rot_mat))};
   return Transform3D{.rotation = rotation / sign(rotation.w), .scale = scale, .translation = translation};
}

Matrix4x4 Transform3D::to_matrix() const
{
   return glm::translate(glm::mat4(1.0f), this->translation) * glm::mat4_cast(this->rotation) * glm::scale(glm::mat4(1.0f), this->scale);
}

Vector3 find_closest_point_between_lines(const Vector3 origin_a, const Vector3 dir_a, const Vector3 origin_b, const Vector3 dir_b)
{
   const auto r = origin_b - origin_a;
   const auto aa = glm::dot(dir_a, dir_a);
   const auto ab = glm::dot(dir_a, dir_b);
   const auto bb = glm::dot(dir_b, dir_b);

   const auto ra = glm::dot(r, dir_a);
   const auto rb = glm::dot(r, dir_b);

   const auto t = rb * ab / bb - ra;
   const auto s = ab * ab / bb - aa;
   if (s == 0) {
      return origin_a;
   }
   return origin_a + (t / s) * dir_a;
}

Vector3 find_closest_point_on_line(Vector3 origin, Vector3 dir, Vector3 point)
{
   const auto ps = point - origin;
   const auto pd = glm::dot(ps, dir);
   const auto vl = glm::length(dir);
   const auto t = pd / vl / vl;
   return origin + t * dir;
}

[[nodiscard]] Vector3 find_point_on_aa_surface(Vector3 origin, Vector3 dir, Axis axis_surface, float surface)
{
   auto t = (surface - vector3_component(origin, axis_surface)) / vector3_component(dir, axis_surface);
   return origin + t * dir;
}

}// namespace triglav