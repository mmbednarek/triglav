#include "stage/GBufferStage.hpp"

#include "BindlessScene.hpp"

#include "triglav/geometry/DebugMesh.hpp"
#include "triglav/geometry/Geometry.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/RenderCore.hpp"

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;

namespace {

geometry::DeviceMesh create_skybox_mesh(graphics_api::Device& device)
{
   auto mesh = geometry::create_box({50, 50, 50});
   mesh.reverse_orientation();
   mesh.triangulate();
   return mesh.upload_to_device(device);
}

}// namespace

GBufferStage::GBufferStage(graphics_api::Device& device, BindlessScene& bindless_scene) :
    m_mesh(create_skybox_mesh(device)),
    m_bindless_scene(bindless_scene)
{
}

void GBufferStage::build_stage(render_core::BuildContext& ctx, const Config& /*config*/) const
{
   ctx.declare_render_target("gbuffer.albedo"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_render_target("gbuffer.position"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_render_target("gbuffer.normal"_name, GAPI_FORMAT(RGBA, Float16));
   ctx.declare_depth_target("gbuffer.depth"_name, GAPI_FORMAT(D, UNorm16));

   ctx.begin_render_pass("gbuffer"_name, "gbuffer.albedo"_name, "gbuffer.position"_name, "gbuffer.normal"_name, "gbuffer.depth"_name);

   this->build_skybox(ctx);
   // this->build_ground(ctx);
   this->build_geometry(ctx);

   ctx.end_render_pass();
}

void GBufferStage::build_skybox(render_core::BuildContext& ctx) const
{
   ctx.bind_vertex_shader("shader/geometry/skybox.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);

   const auto vertex_layout = render_core::vertex_layout_from_components(m_mesh.ranges[0].components);
   ctx.bind_vertex_layout(vertex_layout);

   ctx.bind_fragment_shader("shader/geometry/skybox.fshader"_rc);

   ctx.bind_samplable_texture(1, "engine/texture/skybox.tex"_rc);

   ctx.set_depth_test_mode(graphics_api::DepthTestMode::Disabled);
   ctx.set_is_blending_enabled(false);

   ctx.draw_mesh(m_mesh);
}

void GBufferStage::build_ground(render_core::BuildContext& ctx) const
{
   ctx.bind_vertex_shader("shader/geometry/ground.vshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);

   ctx.bind_fragment_shader("shader/geometry/ground.fshader"_rc);

   ctx.bind_samplable_texture(1, "texture/board.tex"_rc);

   ctx.set_depth_test_mode(graphics_api::DepthTestMode::Disabled);
   ctx.set_vertex_topology(graphics_api::VertexTopology::TriangleStrip);
   ctx.set_is_blending_enabled(false);

   ctx.draw_primitives(4, 0);
}

void GBufferStage::build_geometry(render_core::BuildContext& ctx) const
{
   this->draw_objects_with_material_template(
      ctx, "occlusion_culling.visible_objects.mt0"_external, "shader/bindless_geometry/vertex_vl0.vshader"_rc,
      "shader/bindless_geometry/render_mt0.fshader"_rc, 0, geometry::VertexComponent::Core | geometry::VertexComponent::Texture);
   this->draw_objects_with_material_template(
      ctx, "occlusion_culling.visible_objects.mt1"_external, "shader/bindless_geometry/vertex_vl1.vshader"_rc,
      "shader/bindless_geometry/render_mt1.fshader"_rc, 1,
      geometry::VertexComponent::Core | geometry::VertexComponent::Texture | geometry::VertexComponent::NormalMap);
   this->draw_objects_with_material_template(
      ctx, "occlusion_culling.visible_objects.mt2"_external, "shader/bindless_geometry/vertex_vl1.vshader"_rc,
      "shader/bindless_geometry/render_mt2.fshader"_rc, 2,
      geometry::VertexComponent::Core | geometry::VertexComponent::Texture | geometry::VertexComponent::NormalMap);
}

void GBufferStage::draw_objects_with_material_template(render_core::BuildContext& ctx, const render_core::BufferRef visible_objects,
                                                       VertexShaderName vs_name, const FragmentShaderName fs_name,
                                                       const u32 material_template_index,
                                                       const geometry::VertexComponentFlags vertex_components) const
{
   ctx.bind_vertex_shader(vs_name);

   const auto layout = render_core::vertex_layout_from_components(vertex_components);
   ctx.bind_vertex_layout(layout);
   ctx.bind_uniform_buffer(0, "core.view_properties"_external);
   ctx.bind_storage_buffer(1, visible_objects);

   ctx.bind_fragment_shader(fs_name);

   ctx.bind_sampled_texture_array(2, m_bindless_scene.scene_texture_refs());
   ctx.bind_storage_buffer(3, &m_bindless_scene.material_template_properties(material_template_index));

   ctx.bind_vertex_buffer(&m_bindless_scene.combined_vertex_buffer());
   ctx.bind_index_buffer(&m_bindless_scene.combined_index_buffer());

   ctx.set_is_blending_enabled(false);

   ctx.draw_indexed_indirect_with_count(visible_objects, "occlusion_culling.count_buffer"_external, 80, sizeof(DrawCall),
                                        sizeof(u32) * material_template_index);
}

}// namespace triglav::renderer::stage
