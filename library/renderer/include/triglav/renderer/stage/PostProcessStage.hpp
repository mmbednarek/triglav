#pragma once

#include "IStage.hpp"

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer::stage {

class PostProcessStage final : public IStage
{
 public:
   void build_stage(render_core::BuildContext& ctx) const override;
};

}// namespace triglav::renderer::stage