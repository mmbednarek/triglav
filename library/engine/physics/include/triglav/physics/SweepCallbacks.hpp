#pragma once

#include "triglav/geometry/Geometry.hpp"

namespace triglav::physics {

enum class TraversableKind
{
   Mesh,
   Capsule,
   AABB,
   Terrain,
   Triangle,
   MaxValue
};

struct TraversableObject
{
   TraversableKind kind{};
   Matrix4x4 inverse_transform{};
   const void* payload{};
};

geometry::SweepHit sweep_cast(const TraversableObject* static_object, const TraversableObject* dynamic_object, const Vector3& movement_vec);
bool collision_test(const TraversableObject* static_object, const TraversableObject* dynamic_object, const Vector3& movement_vec);
float ray_cast(const TraversableObject* object, const geometry::Ray& ray);

}// namespace triglav::physics