#include "PhysicsSystem.hpp"

#include "triglav/engine/Engine.hpp"
#include "triglav/world/Level.hpp"

namespace triglav::physics {

using namespace name_literals;

engine::SystemRegisterer PHYSICS_SYSTEM_REGISTERER{{
   .constructor = [](world::Level& /*level*/) -> std::unique_ptr<world::ISystem> { return std::make_unique<PhysicsSystem>(); },
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

PhysicsSystem::PhysicsSystem() = default;

Name PhysicsSystem::system_name()
{
   return TAG;
}

void PhysicsSystem::on_level_loaded(world::Level& level)
{
   std::vector<BVHNode> nodes;
   for (const auto& [entity_id, mesh_comp] : level.all<world::Mesh>()) {
      const auto& mesh = engine::resource_manager().get(mesh_comp.name);
      const auto* transform_ptr = level.component_opt<Transform3D>(entity_id);
      Transform3D transform = transform_ptr == nullptr ? Transform3D::identity() : *transform_ptr;

      nodes.emplace_back(BVHNode{
         .entity_id = entity_id,
         .bbox = mesh.bounding_box.transform(transform.to_matrix()),
      });
   }

   m_tree.build(nodes);
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
      const auto& mesh_comp = level.component<world::Mesh>(entity_id);
      const auto& mesh = engine::resource_manager().get(mesh_comp.name);
      const auto* transform_ptr = level.component_opt<Transform3D>(entity_id);
      Transform3D transform = transform_ptr == nullptr ? Transform3D::identity() : *transform_ptr;

      m_tree.add(BVHNode{
         .entity_id = entity_id,
         .bbox = mesh.bounding_box.transform(transform.to_matrix()),
      });
   }
}

void PhysicsSystem::on_modified_component(world::Level& level, Name /*component_name*/, world::ComponentID /*component_id*/,
                                          std::span<const world::EntityID> entities)
{
   for (const auto entity_id : entities) {
      const auto* mesh_comp = level.component_opt<world::Mesh>(entity_id);
      if (mesh_comp == nullptr)
         continue;

      const auto& mesh = engine::resource_manager().get(mesh_comp->name);
      const auto* transform_ptr = level.component_opt<Transform3D>(entity_id);
      Transform3D transform = transform_ptr == nullptr ? Transform3D::identity() : *transform_ptr;

      m_tree.update(BVHNode{
         .entity_id = entity_id,
         .bbox = mesh.bounding_box.transform(transform.to_matrix()),
      });
   }
}

RayHit PhysicsSystem::trace_ray(const geometry::Ray& ray) const
{
   const auto [distance, node] = m_tree.traverse(ray);
   if (node == nullptr) {
      return RayHit{
         .distance = INFINITY,
         .entity_id = world::NO_ENTITY,
      };
   }

   return {
      .distance = distance,
      .entity_id = node->entity_id,
   };
}

}// namespace triglav::physics
