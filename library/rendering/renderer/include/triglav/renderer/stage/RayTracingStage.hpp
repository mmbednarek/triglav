#pragma once

#include "IStage.hpp"
#include "ShadowMapManager.hpp"

namespace triglav::renderer {
class RayTracingScene;
}

namespace triglav::renderer::stage {

class RayTracingStage : public IStage
{
 public:
   RayTracingStage(RayTracingScene& rt_scene, ShadowMapManager& shadow_map_manager);
   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

 private:
   RayTracingScene& m_rt_scene;
   ShadowMapManager& m_shadow_map_manager;
};


}// namespace triglav::renderer::stage