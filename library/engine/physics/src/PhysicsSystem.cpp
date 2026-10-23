#include "PhysicsSystem.hpp"

#include "triglav/engine/Engine.hpp"
#include "triglav/world/Level.hpp"

#include "Collision.hpp"
#include "Physics.hpp"

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

PhysicsSystem::PhysicsSystem(world::Level& level) :
    m_level(level),
    m_provider(level),
    m_static_bvh(level, m_provider),
    m_dynamic_bvh(level, m_provider)
{
}

Name PhysicsSystem::system_name()
{
   return TAG;
}

void PhysicsSystem::on_level_loaded(world::Level& /*level*/)
{
   m_static_bvh.build_static();
   m_dynamic_bvh.build_dynamic();
}

void PhysicsSystem::on_removed_entities(world::Level& /*level*/, std::span<const world::EntityID> ids)
{
   for (const auto entity_id : ids) {
      m_static_bvh.remove(entity_id);
   }
}

void PhysicsSystem::on_added_component(world::Level& level, Name component_name, world::ComponentID /*component_id*/,
                                       std::span<const world::EntityID> entities)
{
   if (component_name != "triglav::world::Mesh"_name)
      return;

   for (const auto entity_id : entities) {
      if (level.has_component<Movement>(entity_id)) {
         m_dynamic_bvh.add(entity_id);
         continue;
      }

      m_static_bvh.add(entity_id);
   }
}

void PhysicsSystem::on_modified_component(world::Level& level, Name /*component_name*/, world::ComponentID /*component_id*/,
                                          std::span<const world::EntityID> entities)
{
   for (const auto entity_id : entities) {
      if (!level.has_component<world::Mesh>(entity_id))
         continue;
      if (level.has_component<Movement>(entity_id)) {
         m_dynamic_bvh.update(entity_id);
         continue;
      }

      m_static_bvh.update(entity_id);
   }
}

void PhysicsSystem::tick(const float delta_time)
{
   std::vector<std::pair<world::EntityID, world::EntityID>> entities;
   m_dynamic_bvh.find_overlaps(entities);

   std::map<world::EntityID, Vector3> movement_vectors;

   for (const auto [entity_a, entity_b] : entities) {
      auto& mov_a = m_level.mut_component<Movement>(entity_a);
      auto& mov_b = m_level.mut_component<Movement>(entity_b);

      RigidBox box_a{
         .box = m_dynamic_bvh.bounding_box(entity_a),
         .velocity = mov_a.motion,
      };
      RigidBox box_b{
         .box = m_dynamic_bvh.bounding_box(entity_b),
         .velocity = mov_b.motion,
      };
      const auto resolution = resolve_aabb_collision(box_a, box_b, delta_time);
      if (!resolution.hit)
         continue;

      mov_a.motion = resolution.velocity_a;
      mov_b.motion = resolution.velocity_b;
      movement_vectors[entity_a] = resolution.movement_a;
      movement_vectors[entity_b] = resolution.movement_b;
      m_dynamic_bvh.update(entity_a);
      m_dynamic_bvh.update(entity_b);
   }

   for (const auto [entity_id, movement] : m_level.all<Movement>()) {
      auto& movement_mut = m_level.mut_component<Movement>(entity_id);

      if (!movement.is_on_ground) {
         movement_mut.motion += delta_time * Vector3{0, 0, -40.0f};
      }
      movement_mut.motion -= movement_mut.motion * delta_time;

      Vector3 movement_vec;
      if (auto it = movement_vectors.find(entity_id); it != movement_vectors.end()) {
         movement_vec = it->second;
      } else {
         movement_vec = delta_time * movement.motion;
      }

      auto& transform = m_level.mut_component<Transform3D>(entity_id);

      if ((transform.translation.z + movement_vec.z) <= 3.5f) {
         movement_mut.is_on_ground = true;
         movement_mut.motion.z = 0.0f;
         movement_vec.z = 0.0f;
         transform.translation.z = 3.5f;
      } else {
         movement_mut.is_on_ground = false;
      }

      if (movement_vec.length() <= TOLERANCE) {
         continue;
      }

      auto bb = m_dynamic_bvh.bounding_box(entity_id);

      auto response = get_slide_response(m_static_bvh, bb, movement_vec);
      m_level.mut_component<Transform3D>(entity_id).translation += response.movement;
      if (response.normal.z > 0.0f) {
         movement_mut.is_on_ground = true;
         movement_mut.motion.z = 0.0f;
      }
   }
}

LevelBVH::RayHit PhysicsSystem::ray_cast(const geometry::Ray& ray) const
{
   return m_static_bvh.ray_cast(ray);
}

LevelBVH::SweepHit PhysicsSystem::sweep_cast(const geometry::BoundingBox& bb, const Vector3& movement) const
{
   return m_static_bvh.sweep_cast(bb, movement);
}

}// namespace triglav::physics
