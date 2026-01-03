#pragma once

#include "triglav/Int.hpp"
#include "triglav/Math.hpp"

namespace triglav::render_core {
class BuildContext;
class JobGraph;
}// namespace triglav::render_core

namespace triglav::editor {

class IRenderOverlay
{
 public:
   virtual ~IRenderOverlay() = default;

   virtual void build_update_job(render_core::BuildContext& ctx) = 0;
   virtual void build_render_job(render_core::BuildContext& ctx) = 0;
   virtual void update(render_core::JobGraph& graph, u32 frame_index, float delta_time) = 0;
   [[nodiscard]] virtual Vector4 dimensions() const = 0;
};

}// namespace triglav::editor