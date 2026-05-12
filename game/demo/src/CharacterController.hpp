#pragma once

#include "triglav/Logging.hpp"
#include "triglav/Math.hpp"
#include "triglav/renderer/AnimationManager.hpp"
#include "triglav/renderer/Scene.hpp"

namespace demo {

enum class AnalogAction
{
   View,
   Movement,
};

enum class CharacterState
{
   Idle,
   Moving,
};

class CharacterController
{
   TG_DEFINE_LOG_CATEGORY(CharacterController)
 public:
   CharacterController(triglav::renderer::Scene& scene, triglav::renderer::AnimationManager& animation_manager);

   void setup_character();

   void on_analog_action(AnalogAction action, triglav::Vector2 value);
   void stop_character();

   void tick(float delta_time);

 private:
   void on_movement(triglav::Vector2 value);
   void on_view(triglav::Vector2 value);
   void forward_state() const;
   void recalculate_forward_vector();

   // Refs
   triglav::renderer::Scene& m_scene;
   triglav::renderer::AnimationManager& m_animation_manager;

   // State
   CharacterState m_character_state = CharacterState::Idle;
   triglav::renderer::ObjectID m_character_id = triglav::renderer::UNSELECTED_OBJECT;
   triglav::renderer::AnimationID m_character_animation_id = triglav::renderer::NO_ANIMATION;
   triglav::Vector3 m_character_position = {0.0f, 0.0f, 3.5f};
   triglav::Vector3 m_character_forward;
   triglav::Vector2 m_analog_forward;
   float m_camera_pitch = 0.0f, m_camera_yaw = 0.0f;
   triglav::Quaternion m_camera_orientation = {1.0f, 0.0f, 0.0f, 0.0f};
};

}// namespace demo