#include "RenderingJob.hpp"

#include "triglav/render_core/BuildContext.hpp"

namespace triglav::renderer {

using namespace name_literals;

RenderingJob::RenderingJob(const Config config) :
    m_config(config)
{
}

void RenderingJob::build_job(render_core::BuildContext& ctx) const
{
   ctx.reset_timestamp_queries(0, 2);
   ctx.query_timestamp(0, false);

   ctx.reset_pipeline_queries(0, 1);
   ctx.begin_query(0);

   for (auto& stage : m_stages) {
      stage->build_stage(ctx, m_config);
   }

   ctx.end_query(0);

   ctx.query_timestamp(1, true);
}

void RenderingJob::set_config(const Config config)
{
   m_config = config;
}

}// namespace triglav::renderer
