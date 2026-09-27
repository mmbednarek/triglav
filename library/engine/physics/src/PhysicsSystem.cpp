#include "PhysicsSystem.hpp"

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

namespace {

BVHNode bvh_node_from_entity_id(const world::Level& level, const world::EntityID entity_id, const world::Mesh* mesh_comp = nullptr)
{
   if (mesh_comp == nullptr) {
      mesh_comp = level.component_opt<world::Mesh>(entity_id);
      assert(mesh_comp != nullptr);
   }
   const auto& mesh = engine::resource_manager().get(mesh_comp->name);
   const auto* transform_ptr = level.component_opt<Transform3D>(entity_id);
   Transform3D transform = transform_ptr == nullptr ? Transform3D::identity() : *transform_ptr;

   return BVHNode{
      .entity_id = entity_id,
      .bbox = mesh.bounding_box.transform(transform.to_matrix()),
      .inv_transform = glm::inverse(transform.to_matrix()),
   };
}

}// namespace

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
    m_tree(m_provider)
{
}

Name PhysicsSystem::system_name()
{
   return TAG;
}

void PhysicsSystem::on_level_loaded(world::Level& level)
{
   std::vector<BVHNode> nodes;
   nodes.reserve(level.component_count<world::Mesh>());

   for (const auto& [entity_id, mesh_comp] : level.all<world::Mesh>()) {
      nodes.emplace_back(bvh_node_from_entity_id(level, entity_id, &mesh_comp));
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
      m_tree.add(bvh_node_from_entity_id(level, entity_id));
   }
}

void PhysicsSystem::on_modified_component(world::Level& level, Name /*component_name*/, world::ComponentID /*component_id*/,
                                          std::span<const world::EntityID> entities)
{
   for (const auto entity_id : entities) {
      if (!level.has_component<world::Mesh>(entity_id))
         continue;

      m_tree.update(bvh_node_from_entity_id(level, entity_id));
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
