#pragma once

#include "IStage.hpp"

#include "triglav/Name.hpp"

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
   explicit PostProcessStage(UpdateUserInterfaceJob* update_user_interface_job, Name output_render_target);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

 private:
   UpdateUserInterfaceJob* m_update_user_interface_job;
   Name m_output_render_target;
};

}// namespace triglav::renderer::stage