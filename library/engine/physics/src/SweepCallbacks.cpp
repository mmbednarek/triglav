#include "SweepCallbacks.hpp"

#include "triglav/geometry/Geometry.hpp"
#include "triglav/geometry/MeshBVH.hpp"
#include "triglav/geometry/Terrain.hpp"

namespace triglav::physics {

namespace {

using SweepCallback = geometry::SweepHit (*)(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                             const Vector3& movement_vec);

using CollisionTestCallback = bool (*)(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                       const Vector3& movement_vec);

using RayCastCallback = float (*)(const TraversableObject* static_object, const geometry::Ray& ray);

constexpr mem_size OBJECT_COUNT = static_cast<mem_size>(TraversableKind::MaxValue);
constexpr mem_size CALLBACK_COUNT = static_cast<mem_size>(TraversableKind::MaxValue) * static_cast<mem_size>(TraversableKind::MaxValue);

constexpr mem_size pair_index(const TraversableKind a, const TraversableKind b)
{
   return static_cast<mem_size>(a) + static_cast<mem_size>(b) * static_cast<mem_size>(TraversableKind::MaxValue);
}

constexpr std::array<SweepCallback, CALLBACK_COUNT> generate_sweep_callbacks();
constexpr std::array<CollisionTestCallback, CALLBACK_COUNT> generate_collision_test_callbacks();
constexpr std::array<RayCastCallback, OBJECT_COUNT> generate_ray_cast_callbacks();

std::array<SweepCallback, CALLBACK_COUNT> SWEEP_CALLBACKS = generate_sweep_callbacks();
std::array<CollisionTestCallback, CALLBACK_COUNT> COLLISION_TEST_CALLBACKS = generate_collision_test_callbacks();
std::array<RayCastCallback, OBJECT_COUNT> RAY_CAST_CALLBACKS = generate_ray_cast_callbacks();

geometry::SweepHit sweep_cast_aabb_aabb(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                        const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::BoundingBox*>(static_object->payload)->sweep_aabb(local_space_bb, local_space_mov);
   return {res.entry_time, res.normal};
}

geometry::SweepHit sweep_cast_mesh_aabb(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                        const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::MeshBVH*>(static_object->payload)->sweep_cast(local_space_bb, local_space_mov);
   return {res.time, res.normal};
}

geometry::SweepHit sweep_cast_triangle_aabb(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                            const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return geometry::sweep_triangle_aabb(*static_cast<const std::array<Vector3, 3>*>(static_object->payload), local_space_bb,
                                        local_space_mov);
}

geometry::SweepHit sweep_cast_terrain_aabb(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                           const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return static_cast<const geometry::Terrain*>(static_object->payload)->generic_sweep_cast(local_space_bb, local_space_mov);
}

geometry::SweepHit sweep_cast_aabb_capsule(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                           const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::BoundingBox*>(static_object->payload)->sweep_capsule(local_space_cap, local_space_mov);
   return {res.entry_time, res.normal};
}

geometry::SweepHit sweep_cast_capsule_capsule(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                              const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return static_cast<const geometry::Capsule*>(static_object->payload)->sweep(local_space_cap, local_space_mov);
}

geometry::SweepHit sweep_cast_mesh_capsule(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                           const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::MeshBVH*>(static_object->payload)->sweep_cast_capsule(local_space_cap, local_space_mov);
   return {res.time, res.normal};
}

geometry::SweepHit sweep_cast_triangle_capsule(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                               const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return geometry::sweep_triangle_capsule(*static_cast<const std::array<Vector3, 3>*>(static_object->payload), local_space_cap,
                                           local_space_mov);
}

geometry::SweepHit sweep_cast_terrain_capsule(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                              const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::Terrain*>(static_object->payload)->sweep_cast_capsule(local_space_cap, local_space_mov);
   return {res.time, res.normal};
}


constexpr std::array<SweepCallback, CALLBACK_COUNT> generate_sweep_callbacks()
{
   std::array<SweepCallback, CALLBACK_COUNT> result{};
   result[pair_index(TraversableKind::AABB, TraversableKind::AABB)] = sweep_cast_aabb_aabb;
   result[pair_index(TraversableKind::Mesh, TraversableKind::AABB)] = sweep_cast_mesh_aabb;
   result[pair_index(TraversableKind::Triangle, TraversableKind::AABB)] = sweep_cast_triangle_aabb;
   result[pair_index(TraversableKind::Terrain, TraversableKind::AABB)] = sweep_cast_terrain_aabb;
   result[pair_index(TraversableKind::AABB, TraversableKind::Capsule)] = sweep_cast_aabb_capsule;
   result[pair_index(TraversableKind::Capsule, TraversableKind::Capsule)] = sweep_cast_capsule_capsule;
   result[pair_index(TraversableKind::Mesh, TraversableKind::Capsule)] = sweep_cast_mesh_capsule;
   result[pair_index(TraversableKind::Triangle, TraversableKind::Capsule)] = sweep_cast_triangle_capsule;
   result[pair_index(TraversableKind::Terrain, TraversableKind::Capsule)] = sweep_cast_terrain_capsule;
   // mesh
   return result;
}

bool collision_test_aabb_aabb(const TraversableObject* static_object, const TraversableObject* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_bb = static_cast<const geometry::BoundingBox*>(dynamic_object->payload)
                                  ->transform(static_object->inverse_transform)
                                  .extend_by_movement(movement_vec);
   return static_cast<const geometry::BoundingBox*>(static_object->payload)->does_intersect_aabb(local_space_bb);
}

bool collision_test_aabb_capsule(const TraversableObject* static_object, const TraversableObject* dynamic_object,
                                 const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)
                                   ->transform(static_object->inverse_transform)
                                   .extend_by_movement(movement_vec);
   return static_cast<const geometry::BoundingBox*>(static_object->payload)->does_intersect_capsule(local_space_cap);
}

constexpr std::array<CollisionTestCallback, CALLBACK_COUNT> generate_collision_test_callbacks()
{
   std::array<CollisionTestCallback, CALLBACK_COUNT> result{};
   result[pair_index(TraversableKind::AABB, TraversableKind::AABB)] = collision_test_aabb_aabb;
   result[pair_index(TraversableKind::AABB, TraversableKind::Capsule)] = collision_test_aabb_capsule;
   return result;
}

float ray_cast_aabb(const TraversableObject* static_object, const geometry::Ray& ray)
{
   const auto local_space_ray = ray.to_local_space(static_object->inverse_transform);
   return static_cast<const geometry::BoundingBox*>(static_object->payload)->intersect(local_space_ray).min_distance;
}

float ray_cast_mesh(const TraversableObject* static_object, const geometry::Ray& ray)
{
   const auto local_space_ray = ray.to_local_space(static_object->inverse_transform);
   return static_cast<const geometry::MeshBVH*>(static_object->payload)->ray_cast(local_space_ray).distance;
}

float ray_cast_terrain(const TraversableObject* static_object, const geometry::Ray& ray)
{
   const auto local_space_ray = ray.to_local_space(static_object->inverse_transform);
   return static_cast<const geometry::Terrain*>(static_object->payload)->ray_cast(local_space_ray).distance;
}

constexpr std::array<RayCastCallback, OBJECT_COUNT> generate_ray_cast_callbacks()
{
   std::array<RayCastCallback, OBJECT_COUNT> result{};
   result[static_cast<u32>(TraversableKind::AABB)] = ray_cast_aabb;
   result[static_cast<u32>(TraversableKind::Mesh)] = ray_cast_mesh;
   result[static_cast<u32>(TraversableKind::Terrain)] = ray_cast_terrain;
   return result;
}

}// namespace

geometry::SweepHit sweep_cast(const TraversableObject* static_object, const TraversableObject* dynamic_object, const Vector3& movement_vec)
{
   const auto cb = SWEEP_CALLBACKS[pair_index(static_object->kind, dynamic_object->kind)];
   if (cb == nullptr) {
      return geometry::SweepHit::miss();
   }
   return cb(static_object, dynamic_object, movement_vec);
}

bool collision_test(const TraversableObject* static_object, const TraversableObject* dynamic_object, const Vector3& movement_vec)
{
   const auto cb = COLLISION_TEST_CALLBACKS[pair_index(static_object->kind, dynamic_object->kind)];
   if (cb == nullptr) {
      return false;
   }
   return cb(static_object, dynamic_object, movement_vec);
}

float ray_cast(const TraversableObject* object, const geometry::Ray& ray)
{
   const auto cb = RAY_CAST_CALLBACKS[static_cast<u32>(object->kind)];
   if (cb == nullptr) {
      return false;
   }
   return cb(object, ray);
}

}// namespace triglav::physics