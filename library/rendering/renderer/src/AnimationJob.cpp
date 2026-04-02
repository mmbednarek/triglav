#include "AnimationJob.hpp"

#include "AnimationManager.hpp"
#include "BindlessScene.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer {

using namespace name_literals;

struct ChannelStateGPU
{
   float start_time;
   u32 target_transform_id;
   u32 last_keyframe;
   u32 target_keyframe;
   u32 channel_type;
};

struct AnimationStateGPU
{
   float current_time;
   u32 channel_count;
};

static constexpr u32 MAX_CHANNEL_STATE_COUNT = 256;

AnimationJob::AnimationJob(AnimationManager& animation_manager, BindlessScene& bindless_scene) :
    m_animation_manager(animation_manager),
    m_bindless_scene(bindless_scene),
    m_keyframe_buffer(nullptr)
{
}

void AnimationJob::build_job(render_core::BuildContext& ctx) const
{
   TG_DEBUG_LABEL(ctx, "Animation", {0.8f, 0.2f, 0.2f, 1.0f})

   ctx.declare_staging_buffer("animation_job.channel_states_staging"_name, sizeof(ChannelStateGPU) * MAX_CHANNEL_STATE_COUNT);
   ctx.declare_buffer("animation_job.channel_states"_name, sizeof(ChannelStateGPU) * MAX_CHANNEL_STATE_COUNT);
   ctx.declare_staging_buffer("animation_job.state_staging"_name, sizeof(AnimationStateGPU));
   ctx.declare_buffer("animation_job.state"_name, sizeof(AnimationStateGPU));

   ctx.copy_buffer("animation_job.channel_states_staging"_name, "animation_job.channel_states"_name);
   ctx.copy_buffer("animation_job.state_staging"_name, "animation_job.state"_name);

   ctx.bind_compute_shader("shader/bindless_geometry/animation.cshader"_rc);

   ctx.bind_storage_buffer(0, &m_animation_manager.keyframe_buffer());
   ctx.bind_storage_buffer(1, &m_animation_manager.timestamp_buffer());
   ctx.bind_storage_buffer(2, "animation_job.channel_states"_name);
   ctx.bind_uniform_buffer(3, "animation_job.state"_name);
   ctx.bind_storage_buffer(4, &m_bindless_scene.transform_buffer());

   ctx.dispatch({1, 1, 1});
}

void AnimationJob::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   const auto channel_states_mapping =
      GAPI_CHECK(graph.resources().buffer("animation_job.channel_states_staging"_name, frame_index).map_memory());
   auto& states = channel_states_mapping.cast<std::array<ChannelStateGPU, MAX_CHANNEL_STATE_COUNT>>();

   u32 i = 0;
   for (const auto& [id, anim] : m_animation_manager.animation_states()) {
      for (const auto& channel : anim.channels) {
         auto& dst_state = states[i];
         dst_state.start_time = anim.start_time;
         dst_state.last_keyframe = channel.last_keyframe;
         dst_state.target_keyframe = channel.first_keyframe + 1;
         dst_state.target_transform_id = channel.target_transform_id;
         dst_state.channel_type = channel.channel_type;
         ++i;
      }
   }

   const auto state_mapping = GAPI_CHECK(graph.resources().buffer("animation_job.state_staging"_name, frame_index).map_memory());
   state_mapping.cast<AnimationStateGPU>() = AnimationStateGPU{m_animation_manager.current_time(), i};
}

}// namespace triglav::renderer
