#pragma once

#include "triglav/geometry/MeshData.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/renderer/stage/IStage.hpp"

namespace triglav::editor {

class DecalRenderingStage : public renderer::stage::IStage
{
 public:
   explicit DecalRenderingStage(graphics_api::Device& device);

   void build_stage(render_core::BuildContext& ctx, const renderer::Config& config) const override;
   void set_matrix(const Matrix4x4& matrix);
   void set_info(const Vector3& center, const float radius);

 private:
   graphics_api::Device& m_device;
   geometry::DeviceMesh m_cube;
   graphics_api::Buffer m_matrix_buffer;
   graphics_api::Buffer m_info_buffer;
};

}// namespace triglav::editor