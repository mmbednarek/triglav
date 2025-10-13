#pragma once

#include "IStage.hpp"

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer {
class UpdateUserInterfaceJob;
}

namespace triglav::renderer::stage {

class PostProcessStage final : public IStage
{
 public:
   explicit PostProcessStage(UpdateUserInterfaceJob& updateUserInterfaceJob);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

 private:
   UpdateUserInterfaceJob& m_updateUserInterfaceJob;
};

}// namespace triglav::renderer::stage