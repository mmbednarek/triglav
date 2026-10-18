#include "PhysicsSystem.hpp"

#include "Physics.hpp"
#include "triglav/engine/Engine.hpp"
#include "triglav/world/Level.hpp"

namespace triglav::physics {

using namespace name_literals;

engine::SystemRegisterer PHYSICS_SYSTEM_REGISTERER{{
   .constructor = [](world::Level& level) -> std::unique_ptr<world::ISystem> { return std::make_unique<PhysicsSystem>(level); },
   .components =
      std::vector<Name>{
         "triglav::Transform3D"_name,
         "triglav::world::Mesh"_name,
      },
}};

const geometry::BoundingBox& BVHNode::bounding_box() const
{
   return bbox;
}

world::EntityID BVHNode::index() const
{
   return entity_id;
}

Matrix4x4 BVHNode::inv_transform_matrix() const
{
   return inv_transform;
}

PhysicsSystem::PhysicsSystem(world::Level& level) :
    m_provider(level),
    m_tree(level, m_provider)
{
}

Name PhysicsSystem::system_name()
{
   return TAG;
}

void PhysicsSystem::on_level_loaded(world::Level& /*level*/)
{
   m_tree.build();
}

void PhysicsSystem::on_removed_entities(world::Level& /*level*/, std::span<const world::EntityID> ids)
{
   for (const auto entity_id : ids) {
      m_tree.remove(entity_id);
   }
}

void PhysicsSystem::on_added_component(world::Level& level, Name component_name, world::ComponentID /*component_id*/,
                                       std::span<const world::EntityID> entities)
{
   if (component_name != "triglav::world::Mesh"_name)
      return;

   for (const auto entity_id : entities) {
      if (level.has_component<Movement>(entity_id))
         continue;

      m_tree.add(entity_id);
   }
}

void PhysicsSystem::on_modified_component(world::Level& level, Name /*component_name*/, world::ComponentID /*component_id*/,
                                          std::span<const world::EntityID> entities)
{
   for (const auto entity_id : entities) {
      if (!level.has_component<world::Mesh>(entity_id))
         continue;

      m_tree.update(entity_id);
   }
}

RayHit PhysicsSystem::trace_ray(const geometry::Ray& ray) const
{
   const auto [distance, entity_id] = m_tree.ray_cast(ray);
   return {
      .distance = distance,
      .entity_id = entity_id,
   };
}

}// namespace triglav::physics
