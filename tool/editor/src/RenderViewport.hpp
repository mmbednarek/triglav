#pragma once

#include "triglav/render_core/JobGraph.hpp"

namespace triglav::editor {

class RenderViewport
{
public:
   RenderViewport();

   void initialize();
   void update();
   void build_render_job(render_core::BuildContext& ctx);

private:
   render_core::JobGraph& m_jobGraph;
};

}
