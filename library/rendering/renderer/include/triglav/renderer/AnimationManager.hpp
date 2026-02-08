#pragma once

#include "Scene.hpp"

#include "triglav/Name.hpp"
#include "triglav/event/Delegate.hpp"

#include <map>

namespace triglav::renderer {

using AnimationID = u32;

struct AnimationState
{
   AnimationName animation_name;
   ObjectID target_object_id;
   float start_time;
};

class AnimationManager
{
 public:
   TG_EVENT(OnAnimationBegan, AnimationID, const AnimationState&)

   using StateContainer = std::map<AnimationID, AnimationState>;

   AnimationID start_animation(AnimationName animation_name, ObjectID target_object_id);
   float current_time() const;
   u32 channel_count() const;

   const StateContainer& animation_states() const;

 private:
   AnimationID m_top_animation_id = 0;
   std::chrono::system_clock::time_point m_base_time;
   StateContainer m_states;
};

}// namespace triglav::renderer