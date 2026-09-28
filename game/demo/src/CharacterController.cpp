#include "CharacterController.hpp"

#include "../../../library/engine/physics/include/triglav/physics/PhysicsSystem.hpp"
#include "triglav/engine/Engine.hpp"

using namespace triglav::name_literals;
using triglav::Quaternion;
using triglav::Transform3D;
using triglav::Vector2;
using triglav::Vector3;
using triglav::geometry::BoundingBox;

namespace demo {

constexpr float CAMERA_DISTANCE = 16.0f;
constexpr float PLAYER_SPEED = 20.0f;

// constexpr BoundingBox BOUNDING_BOX = {
//    .min = Vector3{-0.2f, -0.2f, 0.0f},
//    .max = Vector3{0.2f, 0.2f, 3.0f},
// };

CharacterController::CharacterController(triglav::renderer::ViewContext& view_context,
                                         triglav::renderer::AnimationManager& animation_manager) :
    m_view_context(view_context),
    m_animation_manager(animation_manager)
{
}

void CharacterController::setup_character()
{
   const auto& simple_mesh = triglav::engine::resource_manager().get("mesh/simple_human.mesh"_rc);
   m_bounding_box = simple_mesh.bounding_box;

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
   auto new_position = m_character_position;

   if (!m_is_on_ground) {
      m_motion += delta_time * Vector3{0, 0, -10.0f};
      new_position += delta_time * m_motion;
   }

   if (m_is_moving) {
      new_position += PLAYER_SPEED * delta_time * m_character_forward;
      m_is_on_ground = false;
   }

   auto transformed_box = m_bounding_box.transform(Transform3D{
      .rotation = Quaternion{1, 0, 0, 0},
      .scale = {1, 1, 1},
      .translation = new_position,
   }
                                                      .to_matrix());

   triglav::engine::system<triglav::physics::PhysicsSystem>().trace_aabb(transformed_box, [&](const triglav::physics::BVHNode& node) {
      if (node.entity_id == this->m_character_id)
         return;
      new_position += node.bounding_box().minimum_translation_vector(transformed_box);
   });

   if (new_position.z <= 3.5f) {
      m_is_on_ground = true;
      m_motion = {};
      new_position.z = 3.5f;
   }

   m_character_position = new_position;
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
   static constexpr float MIN_PITCH = -0.5f * triglav::MATH_PI;
   static constexpr float MAX_PITCH = 0.5f * triglav::MATH_PI;

   m_camera_yaw += value.x * 0.01f;
   m_camera_pitch += value.y * 0.01f;
   m_camera_pitch = std::clamp(m_camera_pitch, MIN_PITCH, MAX_PITCH);

   m_camera_yaw = std::fmod(m_camera_yaw, 2.0f * triglav::MATH_PI);

   // log_debug("pitch: {}, yaw: {}", m_camera_pitch, m_camera_yaw);

   m_camera_orientation = Quaternion{Vector3{m_camera_pitch, 0.0f, m_camera_yaw}};

   if (m_is_moving) {
      this->recalculate_forward_vector();
   }

   this->forward_state();
}

void CharacterController::forward_state() const
{
   // The character is looking at

   const auto rot = glm::rotation(Vector3{0.0f, -1.0f, 0.0f}, glm::normalize(m_character_forward));

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
   m_character_forward = glm::normalize(Vector3{camera_forward.x, camera_forward.y, 0.0f});
}

}// namespace demo