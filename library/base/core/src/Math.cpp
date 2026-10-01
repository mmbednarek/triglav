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
   const auto translation = matrix[3].xyz();

   const auto vec1 = matrix[0].xyz();
   const auto vec2 = matrix[1].xyz();
   const auto vec3 = matrix[2].xyz();

   Vector3 scale{vec1.length(), vec2.length(), vec3.length()};

   // Avoid division by zero
   static constexpr float epsilon = 1e-8f;
   Vector3 col0 = (scale.x > epsilon) ? (vec1 / scale.x) : Vector3{1.0f, 0.0f, 0.0f};
   Vector3 col1 = (scale.y > epsilon) ? (vec2 / scale.y) : Vector3{0.0f, 1.0f, 0.0f};
   Vector3 col2 = (scale.z > epsilon) ? (vec3 / scale.z) : Vector3{0.0f, 0.0f, 1.0f};

   // Handle reflection (negative scale)
   if (col0.cross(col1).dot(col2) < 0.0f) {
      scale.x = -scale.x;
      col0 = Vector3{} - col0;
   }

   // Construct Matrix3x3 explicitly from column vectors
   const Matrix3x3 rot_mat = Matrix3x3{col0, col1, col2};

   Quaternion rotation = Quaternion::from_rotation_matrix(rot_mat).normalize();

   // Canonicalize quaternion representation (keep real part positive)
   if (rotation.w < 0.0f) {
      rotation *= -1.0f;
   }

   return Transform3D{
      .rotation = rotation,
      .scale = scale,
      .translation = translation,
   };
}

Transform3D Transform3D::null()
{
   return Transform3D{.rotation = {0, 0, 0, 0}, .scale = {0, 0, 0}, .translation = {0, 0, 0}};
}

Matrix4x4 Transform3D::to_matrix() const
{
   return Matrix4x4::translation(this->translation) * (Matrix3x3::rotation(this->rotation) * Matrix3x3::scale(this->scale)).extend();
}

Matrix4x4 Transform3D::to_normal_matrix() const
{
   return this->to_matrix().shrink().inverse().transpose().extend();
}

Transform3D Transform3D::combine(const Transform3D& child) const
{
   return {
      .rotation = glm::normalize(this->rotation * child.rotation),
      .scale = this->scale * child.scale,
      .translation = this->translation + this->rotation * (child.translation * this->scale),
   };
}

Vector3 find_closest_point_between_lines(const Vector3 origin_a, const Vector3 dir_a, const Vector3 origin_b, const Vector3 dir_b)
{
   const auto r = origin_b - origin_a;
   const auto aa = dir_a.dot(dir_a);
   const auto ab = dir_a.dot(dir_b);
   const auto bb = dir_b.dot(dir_b);

   const auto ra = r.dot(dir_a);
   const auto rb = r.dot(dir_b);

   const auto t = rb * ab / bb - ra;
   const auto s = ab * ab / bb - aa;
   if (s == 0) {
      return origin_a;
   }
   return origin_a + dir_a * (t / s);
}

Vector3 find_closest_point_on_line(Vector3 origin, Vector3 dir, Vector3 point)
{
   const auto ps = point - origin;
   const auto pd = ps.dot(dir);
   const auto vl = dir.length();
   const auto t = pd / vl / vl;
   return origin + t * dir;
}

[[nodiscard]] Vector3 find_point_on_aa_surface(Vector3 origin, Vector3 dir, Axis axis_surface, float surface)
{
   auto t = (surface - vector3_component(origin, axis_surface)) / vector3_component(dir, axis_surface);
   return origin + t * dir;
}

}// namespace triglav