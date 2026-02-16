#include "AnimationJob.hpp"

#include "AnimationManager.hpp"
#include "BindlessScene.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer {

using namespace name_literals;

struct ChannelState
{
   float start_time;
   u32 target_mesh;
   u32 last_keyframe;
   u32 target_keyframe;
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
   ctx.declare_staging_buffer("animation_job.channel_states_staging"_name, sizeof(ChannelState) * MAX_CHANNEL_STATE_COUNT);
   ctx.declare_buffer("animation_job.channel_states"_name, sizeof(ChannelState) * MAX_CHANNEL_STATE_COUNT);
   ctx.declare_staging_buffer("animation_job.state_staging"_name, sizeof(AnimationStateGPU));
   ctx.declare_buffer("animation_job.state"_name, sizeof(AnimationStateGPU));

   ctx.copy_buffer("animation_job.channel_states_staging"_name, "animation_job.channel_states"_name);
   ctx.copy_buffer("animation_job.state_staging"_name, "animation_job.state"_name);

   ctx.init_buffer("animation_job.keyframes"_name,
                   std::array<Vector4, 5>{Vector4{0.0f, -20.0f, -10.0f, 0.0f}, Vector4{0.0f, 0.0f, -10.0f, 2000.0f},
                                          Vector4{10.0f, 0.0f, 10.0f, 10000.0f}, Vector4{10.0f, 0.0f, -20.0f, 10500.0f},
                                          Vector4{-10.0f, 0.0f, -5.0f, 12500.0f}});

   ctx.bind_compute_shader("shader/bindless_geometry/animation.cshader"_rc);

   ctx.bind_storage_buffer(0, "animation_job.keyframes"_name);
   ctx.bind_storage_buffer(1, "animation_job.channel_states"_name);
   ctx.bind_storage_buffer(2, "animation_job.state"_name);
   ctx.bind_storage_buffer(3, &m_bindless_scene.scene_object_buffer());

   ctx.dispatch({1, 1, 1});
}

void AnimationJob::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   const auto state_mapping = GAPI_CHECK(graph.resources().buffer("animation_job.state_staging"_name, frame_index).map_memory());
   state_mapping.cast<AnimationStateGPU>() = AnimationStateGPU{m_animation_manager.current_time(), m_animation_manager.channel_count()};

   const auto channel_states_mapping =
      GAPI_CHECK(graph.resources().buffer("animation_job.channel_states_staging"_name, frame_index).map_memory());
   auto& states = channel_states_mapping.cast<std::array<ChannelState, MAX_CHANNEL_STATE_COUNT>>();

   auto it = m_animation_manager.animation_states().begin();
   for (MemorySize i = 0; i < m_animation_manager.channel_count(); ++i) {
      auto& dst_state = states[i];
      const auto& src_state = it->second;
      dst_state.start_time = src_state.start_time;
      dst_state.last_keyframe = 4;
      dst_state.target_keyframe = 1;
      dst_state.target_mesh = src_state.target_object_id;
      ++it;
   }
}

}// namespace triglav::renderer
