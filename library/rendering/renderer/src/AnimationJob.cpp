#include "AnimationJob.hpp"

#include "AnimationManager.hpp"
#include "BindlessScene.hpp"

#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer {

using namespace name_literals;

struct ChannelState
{
   float current_time;
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

AnimationJob::AnimationJob(AnimationManager& m_animation_manager, BindlessScene& m_bindless_scene) :
    m_animation_manager(m_animation_manager),
    m_bindless_scene(m_bindless_scene),
    m_keyframe_buffer(nullptr)
{
}

void AnimationJob::build_job(render_core::BuildContext& ctx) const
{
   // ctx.declare_staging_buffer("animation_job.dispatch_buffer"_name, sizeof(Vector3i));
   ctx.declare_buffer("animation_job.channel_states"_name, sizeof(ChannelState) * MAX_CHANNEL_STATE_COUNT);
   ctx.declare_staging_buffer("animation_job.state"_name, sizeof(AnimationStateGPU));

   ctx.bind_compute_shader("shader/bindless_geometry/animation.cshader"_rc);

   ctx.bind_storage_buffer(0, m_keyframe_buffer);
   ctx.bind_storage_buffer(1, "animation_job.channel_states"_name);
   ctx.bind_storage_buffer(1, "animation_job.channel_states"_name);
   ctx.bind_storage_buffer(3, &m_bindless_scene.scene_object_buffer());

   // ctx.dispatch_indirect("animation_job.dispatch_buffer"_name);
   ctx.dispatch({1, 1, 1});
}

void AnimationJob::prepare_frame(render_core::JobGraph& graph, const u32 frame_index)
{
   const auto state = GAPI_CHECK(graph.resources().buffer("animation_job.current_time"_name, frame_index).map_memory());
   state.cast<AnimationStateGPU>() = AnimationStateGPU{m_animation_manager.current_time(), m_animation_manager.channel_count()};
}

}// namespace triglav::renderer
