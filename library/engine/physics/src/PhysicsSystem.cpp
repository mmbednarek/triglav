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

PhysicsSystem::PhysicsSystem() {}

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
      if (transform_ptr == nullptr) {
         nodes.emplace_back(BVHNode{
            .entity_id = entity_id,
            .bbox = mesh.bounding_box,
         });
      } else {
         nodes.emplace_back(BVHNode{
            .entity_id = entity_id,
            .bbox = mesh.bounding_box.transform(transform_ptr->to_matrix()),
         });
      }
   }

   m_tree.build(nodes);
}

void PhysicsSystem::on_removed_entities(world::Level& /*level*/, std::span<const world::EntityID> /*ids*/) {}

void PhysicsSystem::on_added_component(world::Level& /*level*/, Name /*component_name*/, world::ComponentID /*component_id*/,
                                       std::span<const world::EntityID> /*entities*/)
{
}

void PhysicsSystem::on_modified_component(world::Level& /*level*/, Name /*component_name*/, world::ComponentID /*component_id*/,
                                          std::span<const world::EntityID> /*entities*/)
{
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
