#pragma once

#include "IStage.hpp"

#include "triglav/render_objects/Model.hpp"

namespace triglav::graphics_api {
class Device;
}

namespace triglav::renderer::stage {

class GBufferStage final : public IStage
{
 public:
   explicit GBufferStage(graphics_api::Device& device);

   void build_stage(render_core::BuildContext& ctx) const override;

   void build_skybox(render_core::BuildContext& ctx) const;
   void build_ground(render_core::BuildContext& ctx) const;

 private:
   render_objects::GpuMesh m_mesh;
};

}// namespace triglav::renderer::stage
