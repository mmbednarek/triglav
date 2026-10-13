#include "CharacterController.hpp"

#include "../../../library/engine/physics/include/triglav/physics/PhysicsSystem.hpp"
#include "triglav/engine/Engine.hpp"
#include "triglav/physics/Physics.hpp"

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

Vector3 calculate_movement_vector(const triglav::physics::PhysicsSystem& physics, BoundingBox player_box, const Vector3& movement,
                                  Vector3& out_normal)
{
   static constexpr int MAX_ITER = 2;
   static constexpr float TOLERANCE = 1e-8f;
   static constexpr float MARGIN = 1e-4f;

   Vector3 total{};
   Vector3 remaining = movement;
   std::array<Vector3, MAX_ITER> planes{};
   mem_size plane_count = 0;

   for (int iter = 0; iter < MAX_ITER; ++iter) {
      if (remaining.length_sq() < TOLERANCE)
         break;

      const auto hit = physics.trace_swept(player_box, remaining);
      out_normal = hit.normal;

      if (hit.entity_id == triglav::world::NO_ENTITY || hit.hit_time >= 1.0f) {
         player_box.translate(remaining);
         total = total + remaining;
         break;
      }

      const Vector3 advance = remaining * hit.hit_time + hit.normal * MARGIN;
      player_box.translate(advance);
      total = total + advance;

      const Vector3 leftover = remaining * (1.0f - hit.hit_time);
      Vector3 slide = leftover - hit.normal * leftover.dot(hit.normal);

      for (mem_size plane_id = 0; plane_id < plane_count; ++plane_id) {
         if (slide.dot(planes[plane_id]) < 0.0f) {
            Vector3 crease = hit.normal.cross(planes[plane_id]);
            if (crease.length_sq() < TOLERANCE) {
               slide = Vector3{};
               break;
            }
            crease = crease.normalize();
            slide = crease * leftover.dot(crease);

            for (mem_size j = 0; j < plane_count; ++j) {
               if (j != plane_id && slide.dot(planes[j]) < 0.0f) {
                  slide = {};
                  break;
               }
            }
            break;
         }
      }

      planes[plane_count++] = hit.normal;
      remaining = slide;
   }

   return total;
}

}// namespace

CharacterController::CharacterController(triglav::renderer::ViewContext& view_context,
                                         triglav::renderer::AnimationManager& animation_manager) :
    m_view_context(view_context),
    m_animation_manager(animation_manager)
{
}

void CharacterController::setup_character()
{
   // const auto& simple_mesh = triglav::engine::resource_manager().get("mesh/simple_human.mesh"_rc);
   // m_bounding_box = simple_mesh.bounding_box;
   m_bounding_box = BoundingBox{
      .min = {-1.0f, -1.0f, -1.0f},
      .max = {2.0f, 2.0f, 2.0f},
   };

   auto transform = Transform3D::identity();
   transform.translation = m_character_position;
   transform.scale = {5.0f, 5.0f, 5.0f};

   auto* level = triglav::engine::level();
   m_character_id = level->new_entity();
   level->insert_component<triglav::world::Mesh>(m_character_id)->name = "mesh/simple_human.mesh"_rc;
   *level->insert_component<Transform3D>(m_character_id) = transform;
   level->insert_component<triglav::world::Armature>(m_character_id)->name = "armature/simple_human_rig.arm"_rc;
   level->insert_component<triglav::world::EntityLabel>(m_character_id)->label = "Character";
   level->insert_component<triglav::world::Tag>(m_character_id)->tag = "Character"_name;
   level->insert_component<triglav::physics::Movement>(m_character_id)->motion = Vector3{};

   triglav::engine::level()->flush();

   this->forward_state();
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
   m_motion += Vector3{0, 0, 10.0f};
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
   Vector3 move_vector = {};
   if (!m_is_on_ground) {
      m_motion += delta_time * Vector3{0, 0, -40.0f};
   }
   move_vector += delta_time * m_motion;

   if (m_is_moving) {
      move_vector += PLAYER_SPEED * delta_time * m_character_forward;
      m_is_on_ground = false;
   }

   if ((m_character_position.z + move_vector.z) <= 3.5f) {
      move_vector += {0.0f, 0.0f, 3.5f - m_character_position.z - move_vector.z};
      m_motion.z = 0.0f;
      m_is_on_ground = true;
   }
   if (move_vector.length() <= 1e-7f) {
      return;
   }

   auto character_box = m_bounding_box.transform(Transform3D{
      .rotation = Quaternion{1, 0, 0, 0},
      .scale = {1, 1, 1},
      .translation = m_character_position,
   }
                                                    .to_matrix());

   const auto& physics_system = triglav::engine::system<triglav::physics::PhysicsSystem>();
   Vector3 normal{};
   m_character_position += calculate_movement_vector(physics_system, character_box, move_vector, normal);
   if (normal.z > 0.0f) {
      m_is_on_ground = true;
      m_motion.z = 0.0f;
   }

   this->forward_state();
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

   this->forward_state();
}

void CharacterController::forward_state() const
{
   // The character is looking at

   const auto rot = Quaternion::from_oriented_vector(Vector3{0.0f, -1.0f, 0.0f}, m_character_forward);

   auto transform = Transform3D::identity();
   transform.translation = m_character_position;
   transform.rotation = rot;
   transform.scale = Vector3{2.5f, 2.5f, 2.5f};

   triglav::engine::level()->mut_component<Transform3D>(m_character_id) = transform;

   const auto camera_forward = m_camera_orientation * Vector3{0.0f, 1.0f, 0.0f};
   const auto camera_position = m_character_position - CAMERA_DISTANCE * camera_forward;

   m_view_context.set_camera(camera_position, m_camera_orientation);
}

void CharacterController::recalculate_forward_vector()
{
   const auto camera_forward = m_camera_orientation * Vector3{m_analog_forward.y, m_analog_forward.x, 0.0f};
   m_character_forward = Vector3{camera_forward.x, camera_forward.y, 0.0f}.normalize();
}

}// namespace demo