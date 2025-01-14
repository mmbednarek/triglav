#pragma once

#include "IStage.hpp"

namespace triglav::render_core {
class JobGraph;
}

namespace triglav::renderer::stage {

class ShadingStage final : public IStage
{
 public:
   void build_stage(render_core::BuildContext& ctx) const override;

   void prepare_particles(render_core::BuildContext& ctx) const;
   void render_particles(render_core::BuildContext& ctx) const;

   static void initialize_particles(render_core::JobGraph& ctx);

 private:
};

}// namespace triglav::renderer::stage