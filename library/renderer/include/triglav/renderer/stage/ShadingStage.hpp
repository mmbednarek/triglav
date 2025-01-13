#pragma once

#include "IStage.hpp"

namespace triglav::renderer::stage {

class ShadingStage final : public IStage
{
 public:
   void build_stage(render_core::BuildContext& ctx) const override;

 private:
};

}// namespace triglav::renderer::stage