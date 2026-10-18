#pragma once

#include "triglav/Logging.hpp"
#include "triglav/world/World.hpp"

#include "LevelBVH.hpp"
#include "MeshBVHStorage.hpp"

namespace triglav::physics {

struct BVHNode
{
   world::EntityID entity_id;
   geometry::BoundingBox bbox;
   Matrix4x4 inv_transform;

   [[nodiscard]] const geometry::BoundingBox& bounding_box() const;
   [[nodiscard]] world::EntityID index() const;
   [[nodiscard]] Matrix4x4 inv_transform_matrix() const;
};

struct RayHit
{
   float distance;
   world::EntityID entity_id;
};

struct SweptHit
{
   float hit_time;
   Vector3 normal;
   bool is_inside;
   world::EntityID entity_id;
};

class PhysicsSystem : public world::ISystem
{
   TG_DEFINE_LOG_CATEGORY(PhysicsSystem)
 public:
   TG_TAG_CLASS(triglav::physics::PhysicsSystem)

   explicit PhysicsSystem(world::Level& level);

   Name system_name() override;
   void on_level_loaded(world::Level& level) override;
   void on_removed_entities(world::Level& level, std::span<const world::EntityID> ids) override;
   void on_added_component(world::Level& level, Name component_name, world::ComponentID component_id,
                           std::span<const world::EntityID> entities) override;
   void on_modified_component(world::Level& level, Name component_name, world::ComponentID component_id,
                              std::span<const world::EntityID> entities) override;

   [[nodiscard]] RayHit trace_ray(const geometry::Ray& ray) const;

   [[nodiscard]] SweptHit trace_swept(const geometry::BoundingBox& bb, const Vector3& movement) const
   {
      const auto hit = m_tree.sweep_cast(bb, movement);
      return {
         .hit_time = hit.time,
         .normal = hit.normal,
         .is_inside = false,
         .entity_id = hit.entity_id,
      };
   }

 private:
   BVHProvider m_provider;
   LevelBVH m_tree;
};

}// namespace triglav::physics