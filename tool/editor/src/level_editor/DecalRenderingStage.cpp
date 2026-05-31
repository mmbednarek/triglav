#include "DecalRenderingStage.hpp"

#include "triglav/geometry/DebugMesh.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/RenderCore.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace render_core::literals;

using BU = graphics_api::BufferUsage;

namespace {

geometry::DeviceMesh create_decal_box(graphics_api::Device& device)
{
   auto mesh = geometry::create_box({2, 2, 2});
   mesh.triangulate();
   return mesh.upload_to_device(device);
}

}// namespace

DecalRenderingStage::DecalRenderingStage(graphics_api::Device& device) :
    m_device(device),
    m_cube(create_decal_box(device)),
    m_matrix_buffer(GAPI_CHECK(device.create_buffer(BU::TransferDst | BU::UniformBuffer, sizeof(Matrix4x4)))),
    m_info_buffer(GAPI_CHECK(device.create_buffer(BU::TransferDst | BU::UniformBuffer, sizeof(Vector4))))
{
   Matrix4x4 identity(0);
   GAPI_CHECK_STATUS(m_matrix_buffer.write_indirect(&identity, sizeof(Matrix4x4)));
}

void DecalRenderingStage::build_stage(render_core::BuildContext& ctx, const renderer::Config& /*config*/) const
{
   ctx.begin_render_pass("decals"_name, "gbuffer.albedo"_name, "gbuffer.material"_name, "gbuffer.depth"_name);

   ctx.bind_vertex_shader("editor/shader/decal.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);
   ctx.bind_uniform_buffer(1, &m_matrix_buffer);

   const auto vertex_layout = render_core::vertex_layout_from_components(m_cube.ranges[0].components);
   ctx.bind_vertex_layout(vertex_layout);

   ctx.bind_fragment_shader("editor/shader/decal.fshader"_rc);

   ctx.bind_uniform_buffer(2, &m_info_buffer);
   ctx.bind_texture(3, "gbuffer.depth"_name);

   ctx.set_depth_test_mode(graphics_api::DepthTestMode::ReadOnly);

   ctx.draw_mesh(m_cube);

   ctx.end_render_pass();
}

void DecalRenderingStage::set_matrix(const Matrix4x4& matrix)
{
   GAPI_CHECK_STATUS(m_matrix_buffer.write_indirect(&matrix, sizeof(Matrix4x4)));
}

void DecalRenderingStage::set_info(const Vector3& center, const float radius)
{
   const Vector4 value{center, radius};
   GAPI_CHECK_STATUS(m_info_buffer.write_indirect(&value, sizeof(Vector4)));
}

}// namespace triglav::editor