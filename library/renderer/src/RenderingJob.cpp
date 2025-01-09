#include "RenderingJob.hpp"

namespace triglav::renderer {

void RenderingJob::build_job(render_core::BuildContext& ctx) const
{
   for (auto& stage : m_stages) {
      stage->build_stage(ctx);
   }
}

}// namespace triglav::renderer
