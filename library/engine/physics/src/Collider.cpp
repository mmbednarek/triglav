#include "Collider.hpp"

#include "triglav/geometry/Geometry.hpp"
#include "triglav/geometry/MeshBVH.hpp"
#include "triglav/geometry/Terrain.hpp"

namespace triglav::physics {

namespace {

using SweepCallback = geometry::SweepHit (*)(const ColliderRef* static_object, const ColliderRef* dynamic_object,
                                             const Vector3& movement_vec);

using CollisionTestCallback = bool (*)(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec);

using RayCastCallback = float (*)(const ColliderRef* static_object, const geometry::Ray& ray);

constexpr mem_size OBJECT_COUNT = static_cast<mem_size>(ColliderKind::MaxValue);
constexpr mem_size CALLBACK_COUNT = static_cast<mem_size>(ColliderKind::MaxValue) * static_cast<mem_size>(ColliderKind::MaxValue);

constexpr mem_size pair_index(const ColliderKind a, const ColliderKind b)
{
   return static_cast<mem_size>(a) + static_cast<mem_size>(b) * static_cast<mem_size>(ColliderKind::MaxValue);
}

constexpr std::array<SweepCallback, CALLBACK_COUNT> generate_sweep_callbacks();
constexpr std::array<CollisionTestCallback, CALLBACK_COUNT> generate_collision_test_callbacks();
constexpr std::array<RayCastCallback, OBJECT_COUNT> generate_ray_cast_callbacks();

std::array<SweepCallback, CALLBACK_COUNT> SWEEP_CALLBACKS = generate_sweep_callbacks();
std::array<CollisionTestCallback, CALLBACK_COUNT> COLLISION_TEST_CALLBACKS = generate_collision_test_callbacks();
std::array<RayCastCallback, OBJECT_COUNT> RAY_CAST_CALLBACKS = generate_ray_cast_callbacks();

geometry::SweepHit sweep_cast_aabb_aabb(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::BoundingBox*>(static_object->payload)->sweep_aabb(local_space_bb, local_space_mov);
   return {res.entry_time, res.normal};
}

geometry::SweepHit sweep_cast_mesh_aabb(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::MeshBVH*>(static_object->payload)->sweep_cast(local_space_bb, local_space_mov);
   return {res.time, res.normal};
}

geometry::SweepHit sweep_cast_triangle_aabb(const ColliderRef* static_object, const ColliderRef* dynamic_object,
                                            const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return geometry::sweep_triangle_aabb(*static_cast<const std::array<Vector3, 3>*>(static_object->payload), local_space_bb,
                                        local_space_mov);
}

geometry::SweepHit sweep_cast_terrain_aabb(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_bb =
      static_cast<const geometry::BoundingBox*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return static_cast<const geometry::Terrain*>(static_object->payload)->generic_sweep_cast(local_space_bb, local_space_mov);
}

geometry::SweepHit sweep_cast_aabb_capsule(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::BoundingBox*>(static_object->payload)->sweep_capsule(local_space_cap, local_space_mov);
   return {res.entry_time, res.normal};
}

geometry::SweepHit sweep_cast_capsule_capsule(const ColliderRef* static_object, const ColliderRef* dynamic_object,
                                              const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return static_cast<const geometry::Capsule*>(static_object->payload)->sweep(local_space_cap, local_space_mov);
}

geometry::SweepHit sweep_cast_mesh_capsule(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   const auto res = static_cast<const geometry::MeshBVH*>(static_object->payload)->sweep_cast_capsule(local_space_cap, local_space_mov);
   return {res.time, res.normal};
}

geometry::SweepHit sweep_cast_triangle_capsule(const ColliderRef* static_object, const ColliderRef* dynamic_object,
                                               const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)->transform(static_object->inverse_transform);
   const auto local_space_mov = static_object->inverse_transform.shrink() * movement_vec;

   return geometry::sweep_triangle_capsule(*static_cast<const std::array<Vector3, 3>*>(static_object->payload), local_space_cap,
                                           local_space_mov);
}

geometry::SweepHit sweep_cast_terrain_capsule(const ColliderRef* static_object, const ColliderRef* dynamic_object,
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
   result[pair_index(ColliderKind::AABB, ColliderKind::AABB)] = sweep_cast_aabb_aabb;
   result[pair_index(ColliderKind::Mesh, ColliderKind::AABB)] = sweep_cast_mesh_aabb;
   result[pair_index(ColliderKind::Triangle, ColliderKind::AABB)] = sweep_cast_triangle_aabb;
   result[pair_index(ColliderKind::Terrain, ColliderKind::AABB)] = sweep_cast_terrain_aabb;
   result[pair_index(ColliderKind::AABB, ColliderKind::Capsule)] = sweep_cast_aabb_capsule;
   result[pair_index(ColliderKind::Capsule, ColliderKind::Capsule)] = sweep_cast_capsule_capsule;
   result[pair_index(ColliderKind::Mesh, ColliderKind::Capsule)] = sweep_cast_mesh_capsule;
   result[pair_index(ColliderKind::Triangle, ColliderKind::Capsule)] = sweep_cast_triangle_capsule;
   result[pair_index(ColliderKind::Terrain, ColliderKind::Capsule)] = sweep_cast_terrain_capsule;
   // mesh
   return result;
}

bool collision_test_aabb_aabb(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_bb = static_cast<const geometry::BoundingBox*>(dynamic_object->payload)
                                  ->transform(static_object->inverse_transform)
                                  .extend_by_movement(movement_vec);
   return static_cast<const geometry::BoundingBox*>(static_object->payload)->does_intersect_aabb(local_space_bb);
}

bool collision_test_aabb_capsule(const ColliderRef* static_object, const ColliderRef* dynamic_object, const Vector3& movement_vec)
{
   const auto local_space_cap = static_cast<const geometry::Capsule*>(dynamic_object->payload)
                                   ->transform(static_object->inverse_transform)
                                   .extend_by_movement(movement_vec);
   return static_cast<const geometry::BoundingBox*>(static_object->payload)->does_intersect_capsule(local_space_cap);
}

constexpr std::array<CollisionTestCallback, CALLBACK_COUNT> generate_collision_test_callbacks()
{
   std::array<CollisionTestCallback, CALLBACK_COUNT> result{};
   result[pair_index(ColliderKind::AABB, ColliderKind::AABB)] = collision_test_aabb_aabb;
   result[pair_index(ColliderKind::AABB, ColliderKind::Capsule)] = collision_test_aabb_capsule;
   return result;
}

float ray_cast_aabb(const ColliderRef* static_object, const geometry::Ray& ray)
{
   const auto local_space_ray = ray.to_local_space(static_object->inverse_transform);
   return static_cast<const geometry::BoundingBox*>(static_object->payload)->intersect(local_space_ray).min_distance;
}

float ray_cast_mesh(const ColliderRef* static_object, const geometry::Ray& ray)
{
   const auto local_space_ray = ray.to_local_space(static_object->inverse_transform);
   return static_cast<const geometry::MeshBVH*>(static_object->payload)->ray_cast(local_space_ray).distance;
}

float ray_cast_terrain(const ColliderRef* static_object, const geometry::Ray& ray)
{
   const auto local_space_ray = ray.to_local_space(static_object->inverse_transform);
   return static_cast<const geometry::Terrain*>(static_object->payload)->ray_cast(local_space_ray).distance;
}

constexpr std::array<RayCastCallback, OBJECT_COUNT> generate_ray_cast_callbacks()
{
   std::array<RayCastCallback, OBJECT_COUNT> result{};
   result[static_cast<u32>(ColliderKind::AABB)] = ray_cast_aabb;
   result[static_cast<u32>(ColliderKind::Mesh)] = ray_cast_mesh;
   result[static_cast<u32>(ColliderKind::Terrain)] = ray_cast_terrain;
   return result;
}

}// namespace

ColliderRef ColliderRef::capsule(const geometry::Capsule& capsule)
{
   return {
      .kind = ColliderKind::Capsule,
      .inverse_transform = Matrix4x4{1.0f},
      .payload = &capsule,
   };
}

ColliderRef ColliderRef::aabb(const geometry::BoundingBox& bounding_box)
{
   return {
      .kind = ColliderKind::AABB,
      .inverse_transform = Matrix4x4{1.0f},
      .payload = &bounding_box,
   };
}

ColliderRef ColliderRef::mesh(const geometry::MeshBVH& mesh)
{
   return {
      .kind = ColliderKind::Mesh,
      .inverse_transform = Matrix4x4{1.0f},
      .payload = &mesh,
   };
}

ColliderRef ColliderRef::terrain(const geometry::Terrain& terrain)
{
   return {
      .kind = ColliderKind::Terrain,
      .inverse_transform = Matrix4x4{1.0f},
      .payload = &terrain,
   };
}

geometry::BoundingBox ColliderRef::bounding_box() const
{
   switch (this->kind) {
   case ColliderKind::Mesh:
      return static_cast<const geometry::MeshBVH*>(payload)->bounding_box();
   case ColliderKind::Capsule:
      return static_cast<const geometry::Capsule*>(payload)->bounding_box();
   case ColliderKind::AABB:
      return *static_cast<const geometry::BoundingBox*>(payload);
   case ColliderKind::Terrain:
      return static_cast<const geometry::Terrain*>(payload)->bounding_box();
   default:
      return {};
   }
}

void ColliderRef::set_transform(const Transform3D& transform)
{
   this->inverse_transform = transform.to_matrix().inverse();
}

Collider::Collider(ColliderKind kind, const Matrix4x4& inverse_transform, void* payload) :
    kind(kind),
    inverse_transform(inverse_transform),
    payload(payload)
{
}

Collider::~Collider()
{
   if (payload == nullptr)
      return;
   switch (kind) {
   case ColliderKind::Mesh:
      delete static_cast<geometry::MeshBVH*>(payload);
      break;
   case ColliderKind::Capsule:
      delete static_cast<geometry::Capsule*>(payload);
      break;
   case ColliderKind::AABB:
      delete static_cast<geometry::BoundingBox*>(payload);
      break;
   case ColliderKind::Terrain:
      delete static_cast<geometry::Terrain*>(payload);
      break;
   case ColliderKind::Triangle:
      delete static_cast<std::array<Vector3, 3>*>(payload);
      break;
   case ColliderKind::MaxValue:
      break;
   }
   payload = nullptr;
}

Collider::Collider(Collider&& other) noexcept :
    kind(other.kind),
    inverse_transform(std::move(other.inverse_transform)),
    payload(std::exchange(other.payload, nullptr))
{
}

Collider& Collider::operator=(Collider&& other) noexcept
{
   if (this == &other)
      return *this;
   kind = other.kind;
   inverse_transform = std::move(other.inverse_transform);
   payload = std::exchange(other.payload, nullptr);
   return *this;
}

ColliderRef Collider::to_const_ref() const
{
   return ColliderRef{
      .kind = this->kind,
      .inverse_transform = this->inverse_transform,
      .payload = this->payload,
   };
}

geometry::BoundingBox Collider::bounding_box() const
{
   return this->to_const_ref().bounding_box();
}

Collider Collider::capsule(const geometry::Capsule& capsule)
{
   return {ColliderKind::Capsule, Matrix4x4{1.0f}, new geometry::Capsule{capsule}};
}

Collider Collider::aabb(const geometry::BoundingBox& bounding_box)
{
   return {ColliderKind::AABB, Matrix4x4{1.0f}, new geometry::BoundingBox{bounding_box}};
}

void Collider::set_transform(const Transform3D& transform)
{
   this->inverse_transform = transform.to_matrix().inverse();
}

void Collider::translate(const Vector3& translation)
{
   switch (this->kind) {
   case ColliderKind::Capsule:
      static_cast<geometry::Capsule*>(this->payload)->translate(translation);
      break;
   case ColliderKind::AABB:
      static_cast<geometry::BoundingBox*>(this->payload)->translate(translation);
      break;
   default:
      assert(false);
   }
}

geometry::SweepHit sweep_cast(const ColliderRef& static_object, const ColliderRef& dynamic_object, const Vector3& movement_vec)
{
   const auto cb = SWEEP_CALLBACKS[pair_index(static_object.kind, dynamic_object.kind)];
   if (cb == nullptr) {
      return geometry::SweepHit::miss();
   }
   return cb(&static_object, &dynamic_object, movement_vec);
}

bool collision_test(const ColliderRef& static_object, const ColliderRef& dynamic_object, const Vector3& movement_vec)
{
   const auto cb = COLLISION_TEST_CALLBACKS[pair_index(static_object.kind, dynamic_object.kind)];
   if (cb == nullptr) {
      return false;
   }
   return cb(&static_object, &dynamic_object, movement_vec);
}

float ray_cast(const ColliderRef& object, const geometry::Ray& ray)
{
   const auto cb = RAY_CAST_CALLBACKS[static_cast<u32>(object.kind)];
   if (cb == nullptr) {
      return false;
   }
   return cb(&object, ray);
}

}// namespace triglav::physics