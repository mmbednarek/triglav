#include "stage/GBufferStage.hpp"

#include "triglav/geometry/DebugMesh.hpp"
#include "triglav/geometry/Geometry.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/RenderCore.hpp"

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

namespace {

render_objects::GpuMesh create_skybox_mesh(graphics_api::Device& device)
{
   auto mesh = geometry::create_box({50, 50, 50});
   mesh.reverse_orientation();
   mesh.triangulate();
   return mesh.upload_to_device(device).mesh;
}

}// namespace

GBufferStage::GBufferStage(graphics_api::Device& device) :
    m_mesh(create_skybox_mesh(device))
{
}

void GBufferStage::build_stage(render_core::BuildContext& ctx) const
{
   ctx.declare_render_target("core.color_out"_name, GAPI_FORMAT(BGRA, sRGB));
   ctx.declare_render_target("gbuffer.position"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_render_target("gbuffer.normal"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_depth_target("gbuffer.depth"_name, GAPI_FORMAT(D, UNorm16));

   ctx.begin_render_pass("gbuffer"_name, "core.color_out"_name, "gbuffer.position"_name, "gbuffer.normal"_name, "gbuffer.depth"_name);

   this->build_skybox(ctx);
   this->build_ground(ctx);

   ctx.end_render_pass();

   ctx.export_texture("core.color_out"_name, graphics_api::PipelineStage::Transfer, graphics_api::TextureState::TransferSrc,
                      graphics_api::TextureUsage::TransferSrc);
}

void GBufferStage::build_skybox(render_core::BuildContext& ctx) const
{
   ctx.bind_vertex_shader("skybox.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);

   render_core::VertexLayout layout(sizeof(geometry::Vertex));
   layout.add("position"_name, GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location));
   layout.add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv));
   layout.add("normal"_name, GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal));
   ctx.bind_vertex_layout(layout);

   ctx.bind_fragment_shader("skybox.fshader"_rc);

   ctx.bind_samplable_texture(1, "skybox.tex"_rc);

   ctx.set_depth_test_mode(graphics_api::DepthTestMode::Disabled);
   ctx.draw_mesh(m_mesh);
}

void GBufferStage::build_ground(render_core::BuildContext& ctx) const
{
   ctx.bind_vertex_shader("ground.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);

   ctx.bind_fragment_shader("ground.fshader"_rc);

   ctx.bind_samplable_texture(1, "board.tex"_rc);

   ctx.set_vertex_topology(graphics_api::VertexTopology::TriangleStrip);

   ctx.draw_primitives(4, 0);
}

}// namespace triglav::renderer::stage
