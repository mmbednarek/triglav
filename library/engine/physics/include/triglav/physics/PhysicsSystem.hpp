#pragma once

#include "triglav/Logging.hpp"
#include "triglav/geometry/BVHTree.hpp"
#include "triglav/world/World.hpp"

namespace triglav::physics {

struct BVHNode
{
   world::EntityID entity_id;
   geometry::BoundingBox bbox;

   [[nodiscard]] const geometry::BoundingBox& bounding_box() const;
};

struct RayHit
{
   float distance;
   world::EntityID entity_id;
};

class PhysicsSystem : public world::ISystem
{
   TG_DEFINE_LOG_CATEGORY(PhysicsSystem)
 public:
   TG_TAG_CLASS(triglav::physics::PhysicsSystem)

   PhysicsSystem();

   Name system_name() override;
   void on_level_loaded(world::Level& level) override;
   void on_removed_entities(world::Level& level, std::span<const world::EntityID> ids) override;
   void on_added_component(world::Level& level, Name component_name, world::ComponentID component_id,
                           std::span<const world::EntityID> entities) override;
   void on_modified_component(world::Level& level, Name component_name, world::ComponentID component_id,
                              std::span<const world::EntityID> entities) override;

   RayHit trace_ray(const geometry::Ray& ray) const;

 private:
   geometry::BVHTree<BVHNode> m_tree;
};

}// namespace triglav::physics