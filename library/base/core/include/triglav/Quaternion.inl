#pragma once

namespace triglav {

constexpr Quaternion::Quaternion(const float w, const float x, const float y, const float z) :
    x(x),
    y(y),
    z(z),
    w(w)
{
}

constexpr bool Quaternion::operator==(const Quaternion& rhs) const noexcept
{
   return this->w == rhs.w && this->x == rhs.x && this->y == rhs.y && this->z == rhs.z;
}

constexpr Quaternion& Quaternion::operator*=(const Quaternion& rhs) noexcept
{
   *this = *this * rhs;
   return *this;
}

constexpr Quaternion& Quaternion::operator*=(const float mult) noexcept
{
   this->w *= mult;
   this->x *= mult;
   this->y *= mult;
   this->z *= mult;
   return *this;
}

constexpr Quaternion Quaternion::operator*(const Quaternion& rhs) const noexcept
{
   return Quaternion{w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z, w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
                     w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x, w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w};
}

constexpr Quaternion Quaternion::operator/(const float constant) const noexcept
{
   return Quaternion{w / constant, x / constant, y / constant, z / constant};
}

constexpr Quaternion Quaternion::operator-() const noexcept
{
   return {-w, -x, -y, -z};
}

constexpr Vector3 Quaternion::operator*(const Vector3& rhs) const noexcept
{
   const Vector3 t{
      2.0f * (y * rhs.z - z * rhs.y),
      2.0f * (z * rhs.x - x * rhs.z),
      2.0f * (x * rhs.y - y * rhs.x),
   };

   return Vector3{
      rhs.x + w * t.x + (y * t.z - z * t.y),
      rhs.y + w * t.y + (z * t.x - x * t.z),
      rhs.z + w * t.z + (x * t.y - y * t.x),
   };
}

constexpr Vector3 Quaternion::euler_angles() const noexcept
{
   // Roll (x-axis rotation)
   const float sinr_cosp = 2.0f * (w * x + y * z);
   const float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
   const float out_x = std::atan2(sinr_cosp, cosr_cosp);

   // Pitch (y-axis rotation)
   const float sinp = 2.0f * (w * y - z * x);
   const float out_y = std::asin(std::clamp(sinp, -1.0f, 1.0f));

   // Yaw (z-axis rotation)
   const float siny_cosp = 2.0f * (w * z + x * y);
   const float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
   const float out_z = std::atan2(siny_cosp, cosy_cosp);

   return {out_x, out_y, out_z};
}

constexpr Quaternion Quaternion::normalize() const
{
   const auto length = std::sqrt(w * w * +x * x + y * y + z * z);
   return Quaternion{w / length, x / length, y / length, z / length};
}

constexpr Quaternion Quaternion::inverse() const noexcept
{
   const float n2 = x * x + y * y + z * z + w * w;

   // Handle division by zero for zero-quaternions
   if (n2 < EPSILON) {
      return {0.0, 0.0, 0.0, 0.0};
   }

   const float inv_n2 = 1.0f / n2;
   return {-x * inv_n2, -y * inv_n2, -z * inv_n2, w * inv_n2};
}

constexpr Quaternion Quaternion::identity()
{
   return {1.0f, 0.0f, 0.0f, 0.0f};
}

constexpr Quaternion Quaternion::from_euler_angles(const Vector3& euler)
{
   // Half angles in radians
   const float half_x = euler.x * 0.5f;// Roll
   const float half_y = euler.y * 0.5f;// Pitch
   const float half_z = euler.z * 0.5f;// Yaw

   const float cx = std::cos(half_x);
   const float sx = std::sin(half_x);
   const float cy = std::cos(half_y);
   const float sy = std::sin(half_y);
   const float cz = std::cos(half_z);
   const float sz = std::sin(half_z);

   return Quaternion{
      cz * cy * cx + sz * sy * sx,// w
      cz * cy * sx - sz * sy * cx,// x
      cz * sy * cx + sz * cy * sx,// y
      sz * cy * cx - cz * sy * sx // z
   };
}

constexpr Quaternion Quaternion::from_rotation_matrix(const Matrix3x3& r) noexcept
{
   Quaternion q{};
   const float trace = r[0][0] + r[1][1] + r[2][2];

   if (trace > 0.0f) {
      // w is the largest component
      const float s = std::sqrt(trace + 1.0f) * 2.0f;// s = 4 * w
      q.w = 0.25f * s;
      q.x = (r[2][1] - r[1][2]) / s;
      q.y = (r[0][2] - r[2][0]) / s;
      q.z = (r[1][0] - r[0][1]) / s;
   } else if ((r[0][0] > r[1][1]) && (r[0][0] > r[2][2])) {
      // x is the largest component
      const float s = std::sqrt(1.0f + r[0][0] - r[1][1] - r[2][2]) * 2.0f;// s = 4 * x
      q.w = (r[2][1] - r[1][2]) / s;
      q.x = 0.25f * s;
      q.y = (r[0][1] + r[1][0]) / s;
      q.z = (r[0][2] + r[2][0]) / s;
   } else if (r[1][1] > r[2][2]) {
      // y is the largest component
      const float s = std::sqrt(1.0f + r[1][1] - r[0][0] - r[2][2]) * 2.0f;// s = 4 * y
      q.w = (r[0][2] - r[2][0]) / s;
      q.x = (r[0][1] + r[1][0]) / s;
      q.y = 0.25f * s;
      q.z = (r[1][2] + r[2][1]) / s;
   } else {
      // z is the largest component
      const float s = std::sqrt(1.0f + r[2][2] - r[0][0] - r[1][1]) * 2.0f;// s = 4 * z
      q.w = (r[1][0] - r[0][1]) / s;
      q.x = (r[0][2] + r[2][0]) / s;
      q.y = (r[1][2] + r[2][1]) / s;
      q.z = 0.25f * s;
   }

   return q;
}

constexpr Quaternion Quaternion::angle_axis(const float angle, const Vector3& v)
{
   const Vector3 vs = v * std::sin(angle * 0.5f);
   return Quaternion{std::cos(angle * 0.5f), vs.x, vs.y, vs.z};
}

constexpr Quaternion Quaternion::from_oriented_vector(const Vector3& source, const Vector3& target) noexcept
{
   const float cos_theta = source.dot(target);
   Vector3 rotation_axis;

   if (cos_theta >= 1.0f - EPSILON) {
      return identity();
   }

   if (cos_theta < -1.0f + EPSILON) {
      rotation_axis = Vector3(0, 0, 1).cross(source);
      if (rotation_axis.length() < EPSILON)
         rotation_axis = Vector3(1, 0, 0).cross(source);

      rotation_axis = rotation_axis.normalize();
      return angle_axis(PI, rotation_axis);
   }

   rotation_axis = source.cross(target);

   const float s = std::sqrt((1.0f + cos_theta) * 2.0f);
   const float invs = 1.0f / s;

   return {s * 0.5f, rotation_axis.x * invs, rotation_axis.y * invs, rotation_axis.z * invs};
}

}// namespace triglav