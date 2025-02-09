#pragma once

#include "IStage.hpp"

#include "triglav/render_objects/Model.hpp"

namespace triglav::graphics_api {
class Device;
}

namespace triglav::renderer {
class BindlessScene;
}

namespace triglav::renderer::stage {

class GBufferStage final : public IStage
{
 public:
   GBufferStage(graphics_api::Device& device, BindlessScene& bindlessScene);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

   void build_skybox(render_core::BuildContext& ctx) const;
   void build_ground(render_core::BuildContext& ctx) const;
   void build_geometry(render_core::BuildContext& ctx) const;

 private:
   render_objects::GpuMesh m_mesh;
   BindlessScene& m_bindlessScene;
};

}// namespace triglav::renderer::stage
