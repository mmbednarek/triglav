#include "RenderingJob.hpp"

namespace triglav::renderer {

RenderingJob::RenderingJob(const Config config) :
    m_config(config)
{
}

void RenderingJob::build_job(render_core::BuildContext& ctx) const
{
   for (auto& stage : m_stages) {
      stage->build_stage(ctx, m_config);
   }
}

void RenderingJob::set_config(const Config config)
{
   m_config = config;
}

}// namespace triglav::renderer
