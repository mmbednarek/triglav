#pragma once

#include "triglav/graphics_api/Buffer.hpp"

namespace triglav::render_core {

class BuildContext;
class JobGraph;

}// namespace triglav::render_core
namespace triglav::renderer {

class AnimationManager;
class BindlessScene;

class AnimationJob
{
 public:
   AnimationJob(AnimationManager& m_animation_manager, BindlessScene& m_bindless_scene);

   void build_job(render_core::BuildContext& ctx) const;

   void prepare_frame(render_core::JobGraph& graph, u32 frame_index);

 private:
   AnimationManager& m_animation_manager;
   BindlessScene& m_bindless_scene;
   graphics_api::Buffer* m_keyframe_buffer;
};

}// namespace triglav::renderer