#include "CharacterController.hpp"

using triglav::renderer::SceneObject;
using namespace triglav::name_literals;
using triglav::Quaternion;
using triglav::Transform3D;
using triglav::Vector2;
using triglav::Vector3;

namespace demo {

constexpr float CAMERA_DISTANCE = 16.0f;
constexpr float PLAYER_SPEED = 20.0f;

CharacterController::CharacterController(triglav::renderer::Scene& scene, triglav::renderer::AnimationManager& animation_manager) :
    m_scene(scene),
    m_animation_manager(animation_manager)
{
}

void CharacterController::setup_character()
{
   auto transform = Transform3D::identity();
   transform.translation = m_character_position;
   transform.scale = {5.0f, 5.0f, 5.0f};

   m_character_id = m_scene.add_object(SceneObject{
      .model = "mesh/simple_human.mesh"_rc,
      .name = "Character",
      .transform = transform,
      .armature = "armature/simple_human_rig.arm"_rc,
   });

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
   if (m_character_state != CharacterState::Moving)
      return;

   m_character_state = CharacterState::Idle;
   if (m_character_animation_id != triglav::renderer::NO_ANIMATION) {
      m_animation_manager.stop_animation(m_character_animation_id);
      m_character_animation_id = triglav::renderer::NO_ANIMATION;
   }
}

void CharacterController::tick(const float delta_time)
{
   if (m_character_state == CharacterState::Idle)
      return;

   m_character_position += PLAYER_SPEED * delta_time * m_character_forward;
   this->forward_state();
}

void CharacterController::on_movement(const triglav::Vector2 value)
{
   m_analog_forward = value;
   this->recalculate_forward_vector();

   // Start animation
   if (m_character_state != CharacterState::Moving) {
      m_character_state = CharacterState::Moving;
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

   if (m_character_state == CharacterState::Moving) {
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

   m_scene.set_transform(m_character_id, transform);

   const auto camera_forward = m_camera_orientation * Vector3{0.0f, 1.0f, 0.0f};
   const auto camera_position = m_character_position - CAMERA_DISTANCE * camera_forward;

   m_scene.set_camera(camera_position, m_camera_orientation);

   m_scene.update_shadow_maps();
   m_scene.send_view_changed();
}

void CharacterController::recalculate_forward_vector()
{
   const auto camera_forward = m_camera_orientation * Vector3{m_analog_forward.y, m_analog_forward.x, 0.0f};
   m_character_forward = glm::normalize(Vector3{camera_forward.x, camera_forward.y, 0.0f});
}

}// namespace demo