#pragma once

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
   static constexpr auto JobName = make_name_id("job.rendering");

   void build_job(render_core::BuildContext& ctx) const;

   template<typename TStage, typename... TArgs>
   void emplace_stage(TArgs&&... args)
   {
      m_stages.emplace_back(std::make_unique<TStage>(std::forward<TArgs>(args)...));
   }

 private:
   std::vector<std::unique_ptr<stage::IStage>> m_stages;
};

}// namespace triglav::renderer
