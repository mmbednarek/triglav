#pragma once

#include "IStage.hpp"

namespace triglav::renderer {
class RayTracingScene;
}

namespace triglav::renderer::stage {

class RayTracingStage : public IStage
{
 public:
   explicit RayTracingStage(RayTracingScene& rtScene);
   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

 private:
   RayTracingScene& m_rtScene;
};


}// namespace triglav::renderer::stage