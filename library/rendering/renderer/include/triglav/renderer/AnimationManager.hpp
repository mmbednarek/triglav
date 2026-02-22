#pragma once

#include "Scene.hpp"

#include "triglav/Name.hpp"
#include "triglav/event/Delegate.hpp"

#include <map>

namespace triglav::renderer {

using AnimationID = u32;

struct ChannelState
{
   u32 first_keyframe;
   u32 last_keyframe;
   u32 channel_type;
};

struct AnimationState
{
   AnimationName animation_name;
   ObjectID target_object_id;
   u32 last_frame_id;
   float start_time;
   std::vector<ChannelState> channels;
};

class AnimationManager
{
 public:
   TG_EVENT(OnAnimationBegan, AnimationID, const AnimationState&)

   AnimationManager(graphics_api::Device& device, resource::ResourceManager& resource_manager);

   using StateContainer = std::map<AnimationID, AnimationState>;

   AnimationID start_animation(AnimationName animation_name, ObjectID target_object_id);
   float current_time() const;
   u32 channel_count() const;
   graphics_api::Buffer& keyframe_buffer();
   const graphics_api::Buffer& keyframe_buffer() const;

   graphics_api::Buffer& timestamp_buffer();
   const graphics_api::Buffer& timestamp_buffer() const;

   const StateContainer& animation_states() const;

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resource_manager;
   AnimationID m_top_animation_id = 0;
   std::chrono::system_clock::time_point m_base_time;
   StateContainer m_states;
   graphics_api::Buffer m_stage_buffer;
   graphics_api::Buffer m_keyframe_buffer;
   graphics_api::Buffer m_timestamp_buffer;
   MemorySize m_keyframe_offset = 0;
   MemorySize m_timestamp_offset = 0;
};

}// namespace triglav::renderer