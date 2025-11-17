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
   explicit PostProcessStage(UpdateUserInterfaceJob* update_user_interface_job);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

 private:
   UpdateUserInterfaceJob* m_update_user_interface_job;
};

}// namespace triglav::renderer::stage