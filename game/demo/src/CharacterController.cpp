#include "CharacterController.hpp"

#include "../../../library/engine/physics/include/triglav/physics/PhysicsSystem.hpp"
#include "triglav/engine/Engine.hpp"
#include "triglav/physics/Physics.hpp"
#include "triglav/world/Archetypes.hpp"

using namespace triglav::name_literals;
using triglav::mem_size;
using triglav::Quaternion;
using triglav::Transform3D;
using triglav::Vector2;
using triglav::Vector3;
using triglav::geometry::BoundingBox;

namespace demo {

namespace {

constexpr float CAMERA_DISTANCE = 16.0f;
constexpr float PLAYER_SPEED = 20.0f;
constexpr float CAMERA_OFFSET = 0.5f;

}// namespace

CharacterController::CharacterController(triglav::renderer::ViewContext& view_context,
                                         triglav::renderer::AnimationManager& animation_manager) :
    m_view_context(view_context),
    m_animation_manager(animation_manager)
{
}

void CharacterController::setup_character()
{
   const auto& mesh = triglav::engine::resource_manager().get("mesh/simple_human.mesh"_rc);
   m_bounding_box = mesh.bounding_box;
   // m_bounding_box = BoundingBox{
   //    .min = {-1.0f, -1.0f, -3.25f},
   //    .max = {2.0f, 2.0f, 2.0f},
   // };

   auto transform = Transform3D::identity();
   transform.translation = {20.0f, 0.0f, 3.5f};
   transform.scale *= 5.0f;

   auto* level = triglav::engine::level();
   assert(level != nullptr);

   m_character_id = level->create_archetype<triglav::world::SkeletalMesh>({
      .label = "Character",
      .mesh_name = "mesh/simple_human.mesh"_rc,
      .transform = transform,
      .armature_name = "armature/simple_human_rig.arm"_rc,
   });
   level->insert_component<triglav::physics::Movement>(m_character_id)->motion = Vector3{};

   auto ball_transform = Transform3D::identity();
   ball_transform.translation = Vector3{10.0f, 0, 100.0f};
   ball_transform.scale *= 3.0f;

   m_table_id = level->create_archetype<triglav::world::StaticMesh>({
      .label = "Ball",
      .mesh_name = "mesh/ball.mesh"_rc,
      .transform = ball_transform,
   });
   level->insert_component<triglav::physics::Movement>(m_table_id)->motion = Vector3{0.0f, 0.0f, 0.1f};

   triglav::engine::level()->flush();

   this->forward_state(level->mut_component<Transform3D>(m_character_id));
}

void CharacterController::on_analog_action(const AnalogAction action, const triglav::Vector2 value)
{
   switch (action) {
   case AnalogAction::View:
      this->on_view(value);
      break;
   case AnalogAction::Movement:
      this->on_movement(value);
      break;
   }
}

void CharacterController::jump()
{
   auto* level = triglav::engine::level();
   assert(level != nullptr);

   auto& movement = level->mut_component<triglav::physics::Movement>(m_character_id);
   movement.motion += Vector3{0, 0, 10.0f};
   movement.is_on_ground = false;
}

void CharacterController::stop_character()
{
   if (!m_is_moving)
      return;

   m_is_moving = false;
   if (m_character_animation_id != triglav::renderer::NO_ANIMATION) {
      m_animation_manager.stop_animation(m_character_animation_id);
      m_character_animation_id = triglav::renderer::NO_ANIMATION;
   }
}

void CharacterController::tick(const float delta_time)
{
   triglav::engine::system<triglav::physics::PhysicsSystem>().tick(delta_time);

   auto* level = triglav::engine::level();
   assert(level != nullptr);

   Transform3D& transform = level->mut_component<Transform3D>(m_character_id);
   this->forward_state(transform);

   triglav::physics::Movement& movement = level->mut_component<triglav::physics::Movement>(m_character_id);
   if (!m_is_moving) {
      movement.motion.x = 0.0f;
      movement.motion.y = 0.0f;
   } else {
      movement.motion.x = PLAYER_SPEED * m_character_forward.x;
      movement.motion.y = PLAYER_SPEED * m_character_forward.y;
   }
}

void CharacterController::on_movement(const triglav::Vector2 value)
{
   m_analog_forward = value;
   this->recalculate_forward_vector();

   // Start animation
   if (!m_is_moving) {
      m_is_moving = true;
      m_character_animation_id = m_animation_manager.start_animation("animation/simple_human_walk.anim"_rc, m_character_id, true);
   }
}

void CharacterController::on_view(const Vector2 value)
{
   static constexpr float MIN_PITCH = -0.5f * triglav::PI;
   static constexpr float MAX_PITCH = 0.5f * triglav::PI;

   m_camera_yaw += value.x * 0.01f;
   m_camera_pitch += value.y * 0.01f;
   m_camera_pitch = std::clamp(m_camera_pitch, MIN_PITCH, MAX_PITCH);

   m_camera_yaw = std::fmod(m_camera_yaw, 2.0f * triglav::PI);

   // log_debug("pitch: {}, yaw: {}", m_camera_pitch, m_camera_yaw);

   m_camera_orientation = Quaternion::from_euler_angles({m_camera_pitch, 0.0f, m_camera_yaw});

   if (m_is_moving) {
      this->recalculate_forward_vector();
   }

   this->forward_state(triglav::engine::level()->mut_component<Transform3D>(m_character_id));
}

void CharacterController::forward_state(Transform3D& transform) const
{
   // The character is looking at
   const auto rot = Quaternion::from_oriented_vector(Vector3{0.0f, -1.0f, 0.0f}, m_character_forward);

   transform.rotation = rot;
   transform.scale = Vector3{2.5f, 2.5f, 2.5f};

   const auto camera_forward = m_camera_orientation * Vector3{0.0f, 1.0f, 0.0f};
   triglav::geometry::Ray camera_ray{
      .origin = transform.translation,
      .direction = -camera_forward,
      .distance = CAMERA_DISTANCE,
   };
   const auto hit = triglav::engine::system<triglav::physics::PhysicsSystem>().ray_cast(camera_ray);
   const float cam_distance = (hit.entity_id != triglav::world::NO_ENTITY ? hit.distance : CAMERA_DISTANCE) - CAMERA_OFFSET;
   const auto camera_position = transform.translation - cam_distance * camera_forward;

   m_view_context.set_camera(camera_position, m_camera_orientation);
}

void CharacterController::recalculate_forward_vector()
{
   const auto camera_forward = m_camera_orientation * Vector3{m_analog_forward.y, m_analog_forward.x, 0.0f};
   m_character_forward = Vector3{camera_forward.x, camera_forward.y, 0.0f}.normalize();
}

}// namespace demo