#pragma once

#include "triglav/render_core/JobGraph.hpp"

namespace triglav::editor {

class RenderViewport
{
 public:
   explicit RenderViewport(Vector4 dimensions);
   void initialize(render_core::JobGraph& jobGraph);
   void update();
   void build_render_job(render_core::BuildContext& ctx);

 private:
   Vector4 m_dimensions{};
};

}// namespace triglav::editor
