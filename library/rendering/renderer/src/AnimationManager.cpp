#include "AnimationManager.hpp"

namespace triglav::renderer {

AnimationID AnimationManager::start_animation(const AnimationName animation_name, const ObjectID target_object_id)
{
   const auto id = m_top_animation_id++;
   m_states[id] = AnimationState{
      .animation_name = animation_name,
      .target_object_id = target_object_id,
      .start_time = this->current_time(),
   };

   event_OnAnimationBegan.publish(id, m_states.at(id));
   return id;
}

float AnimationManager::current_time() const
{
   return static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - m_base_time).count()) /
          1000000.0f;
}

u32 AnimationManager::channel_count() const
{
   return static_cast<u32>(m_states.size());
}

const AnimationManager::StateContainer& AnimationManager::animation_states() const
{
   return m_states;
}

}// namespace triglav::renderer
