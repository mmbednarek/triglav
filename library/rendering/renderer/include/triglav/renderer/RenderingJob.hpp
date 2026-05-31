#pragma once

#include "Config.hpp"
#include "stage/IStage.hpp"

#include "triglav/Name.hpp"

#include <memory>
#include <vector>

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer {

class RenderingJob
{
 public:
   explicit RenderingJob(Config config);
   static constexpr auto JobName = make_name_id("job.rendering");

   void build_job(render_core::BuildContext& ctx) const;
   void set_config(Config config);

   template<typename TStage, typename... TArgs>
   TStage& emplace_stage(TArgs&&... args)
   {
      m_stages.emplace_back(std::make_unique<TStage>(std::forward<TArgs>(args)...));
      return *dynamic_cast<TStage*>(m_stages.back().get());
   }

 private:
   std::vector<std::unique_ptr<stage::IStage>> m_stages;
   Config m_config;
};

}// namespace triglav::renderer
