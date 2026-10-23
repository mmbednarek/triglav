#pragma once

#include "triglav/Logging.hpp"
#include "triglav/world/World.hpp"

#include "LevelBVH.hpp"
#include "MeshBVHStorage.hpp"

namespace triglav::physics {

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
   void tick(float delta_time);

   [[nodiscard]] LevelBVH::RayHit ray_cast(const geometry::Ray& ray) const;
   [[nodiscard]] LevelBVH::SweepHit sweep_cast(const geometry::BoundingBox& bb, const Vector3& movement) const;

 private:
   world::Level& m_level;
   BVHProvider m_provider;
   LevelBVH m_static_bvh;
   LevelBVH m_dynamic_bvh;
};

}// namespace triglav::physics