#pragma once

#include "triglav/geometry/Geometry.hpp"

namespace triglav::geometry {
class MeshBVH;
}

namespace triglav::physics {

enum class ColliderKind
{
   Mesh,
   Capsule,
   AABB,
   Terrain,
   Triangle,
   MaxValue
};

struct ColliderRef
{
   ColliderKind kind{};
   Matrix4x4 inverse_transform{};
   const void* payload{};

   static ColliderRef capsule(const geometry::Capsule& capsule);
   static ColliderRef aabb(const geometry::BoundingBox& bounding_box);
   static ColliderRef mesh(const geometry::MeshBVH& mesh);
   static ColliderRef terrain(const geometry::Terrain& terrain);

   [[nodiscard]] geometry::BoundingBox bounding_box() const;

   void set_transform(const Transform3D& transform);
};

struct Collider
{
   ColliderKind kind{};
   Matrix4x4 inverse_transform{};
   void* payload{};

   Collider(ColliderKind kind, const Matrix4x4& inverse_transform, void* payload);

   ~Collider();

   Collider(Collider&& other) noexcept;
   Collider& operator=(Collider&& other) noexcept;
   TG_DELETE_COPY(Collider)


   [[nodiscard]] ColliderRef to_const_ref() const;
   [[nodiscard]] geometry::BoundingBox bounding_box() const;

   static Collider capsule(const geometry::Capsule& capsule);
   static Collider aabb(const geometry::BoundingBox& bounding_box);
   void set_transform(const Transform3D& transform);
   void translate(const Vector3& translation);
};

geometry::SweepHit sweep_cast(const ColliderRef& static_object, const ColliderRef& dynamic_object, const Vector3& movement_vec);
bool collision_test(const ColliderRef& static_object, const ColliderRef& dynamic_object, const Vector3& movement_vec);
float ray_cast(const ColliderRef& object, const geometry::Ray& ray);

}// namespace triglav::physics