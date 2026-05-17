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

struct TerrainVertex
{
   Vector3 position;
   Vector2 uv;
};

const render_core::VertexLayout terrain_layout = render_core::VertexLayout{sizeof(TerrainVertex)}
                                                    .add("position"_name, GAPI_FORMAT(RGB, Float32), offsetof(TerrainVertex, position))
                                                    .add("uv"_name, GAPI_FORMAT(RG, Float32), offsetof(TerrainVertex, uv));

graphics_api::Texture generate_terrain_bitmap(graphics_api::Device& device, const u32 w, const u32 h)
{
   std::vector<float> terrain_vertices(w * h);
   for (u32 y = 0; y < h; ++y) {
      for (u32 x = 0; x < w; ++x) {
         const float xx = 2.0f * (static_cast<float>(x) / static_cast<float>(w)) - 1.0f;
         const float yy = 2.0f * (static_cast<float>(y) / static_cast<float>(h)) - 1.0f;

         terrain_vertices[y * w + x] = xx * xx + yy * yy;
      }
   }

   auto tex = GAPI_CHECK(device.create_texture(GAPI_FORMAT(R, Float32), graphics_api::Resolution{w, h}));
   GAPI_CHECK_STATUS(tex.write(device, reinterpret_cast<const uint8_t*>(terrain_vertices.data())));

   tex.sampler_properties().address_u = graphics_api::TextureAddressMode::Clamp;
   tex.sampler_properties().address_v = graphics_api::TextureAddressMode::Clamp;
   tex.sampler_properties().address_w = graphics_api::TextureAddressMode::Clamp;
   return tex;
}

graphics_api::Buffer generate_terrain_vertices(graphics_api::Device& device)
{
   constexpr float SIZE = 120.0;
   const std::array<TerrainVertex, 4> verts{
      TerrainVertex{Vector3{-SIZE / 2.0f, -SIZE / 2.0f, 0}, Vector2{0.0f, 0.0f}},
      TerrainVertex{Vector3{-SIZE / 2.0f, SIZE / 2.0f, 0}, Vector2{0.0f, 1.0f}},
      TerrainVertex{Vector3{SIZE / 2.0f, SIZE / 2.0f, 0}, Vector2{1.0f, 1.0f}},
      TerrainVertex{Vector3{SIZE / 2.0f, -SIZE / 2.0f, 0}, Vector2{1.0f, 0.0f}},
   };

   auto buff = GAPI_CHECK(device.create_buffer(graphics_api::BufferUsage::TransferDst | graphics_api::BufferUsage::VertexBuffer,
                                               verts.size() * sizeof(TerrainVertex)));
   GAPI_CHECK_STATUS(buff.write_indirect(verts.data(), verts.size() * sizeof(TerrainVertex)));
   return buff;
}


}// namespace

GBufferStage::GBufferStage(graphics_api::Device& device, BindlessScene& bindless_scene) :
    m_mesh(create_skybox_mesh(device)),
    m_terrain_texture(generate_terrain_bitmap(device, 1024, 1024)),
    m_terrain_vertices(generate_terrain_vertices(device)),
    m_bindless_scene(bindless_scene)
{
}

void GBufferStage::build_stage(render_core::BuildContext& ctx, const Config& /*config*/) const
{
   ctx.declare_render_target("gbuffer.albedo"_name, GAPI_FORMAT(RGBA, UNorm8));
   ctx.declare_render_target("gbuffer.material"_name, GAPI_FORMAT(RG, UNorm8));
   ctx.declare_render_target("gbuffer.normal"_name, GAPI_FORMAT(RGBA, UNorm8));
   ctx.declare_depth_target("gbuffer.depth"_name, GAPI_FORMAT(D, Float32));

   ctx.begin_render_pass("gbuffer"_name, "gbuffer.albedo"_name, "gbuffer.material"_name, "gbuffer.normal"_name, "gbuffer.depth"_name);

   this->build_skybox(ctx);
   // this->build_ground(ctx);
   this->build_terrain(ctx);
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
   for (const auto& info : render_objects::GEOMETRY_RENDER_INFOS) {
      this->draw_objects_with_render_info(ctx, info);
   }
}

void GBufferStage::build_terrain(render_core::BuildContext& ctx) const
{
   ctx.bind_vertex_shader("shader/terrain/vertex.vshader"_rc);

   ctx.bind_hull_shader("shader/terrain/hull.hshader"_rc);

   ctx.bind_uniform_buffer(0, "core.view_properties"_external);

   ctx.bind_domain_shader("shader/terrain/domain.dshader"_rc);

   ctx.bind_uniform_buffer(1, "core.view_properties"_external);
   ctx.bind_samplable_texture(2, &m_terrain_texture);

   ctx.bind_fragment_shader("shader/terrain/fragment.fshader"_rc);

   ctx.set_vertex_topology(graphics_api::VertexTopology::PatchList);

   ctx.bind_vertex_layout(terrain_layout);
   ctx.bind_vertex_buffer(&m_terrain_vertices);
   ctx.set_tesselation_control_points(4);

   ctx.draw_primitives(4, 0, 1, 0);
}

void GBufferStage::draw_objects_with_render_info(render_core::BuildContext& ctx,
                                                 const render_objects::MaterialGeometryRenderInfo& info) const
{
   const auto& vertex_layout_info = render_objects::VERTEX_LAYOUT_INFOS[info.vertex_layout_id];

   ctx.bind_vertex_shader(vertex_layout_info.vertex_shader);

   const auto layout = render_core::vertex_layout_from_components(vertex_layout_info.components);
   ctx.bind_vertex_layout(layout);
   ctx.bind_uniform_buffer(0, "core.view_properties"_external);
   ctx.bind_storage_buffer(1, render_core::External(info.draw_call_buffer));
   if (vertex_layout_info.components & geometry::VertexComponent::Skeleton) {
      ctx.bind_storage_buffer(4, &m_bindless_scene.transform_matrix_buffer());
   }

   ctx.bind_fragment_shader(info.fragment_shader);

   ctx.bind_sampled_texture_array(2, m_bindless_scene.scene_texture_refs());
   ctx.bind_storage_buffer(3, &m_bindless_scene.material_template_properties(info.materialPropIndex));

   ctx.bind_vertex_buffer(&m_bindless_scene.combined_vertex_buffer());
   ctx.bind_index_buffer(&m_bindless_scene.combined_index_buffer());

   ctx.set_is_blending_enabled(false);

   ctx.draw_indexed_indirect_with_count(render_core::External(info.draw_call_buffer), "occlusion_culling.count_buffer"_external, 80,
                                        sizeof(DrawCall), sizeof(u32) * info.index);
}

}// namespace triglav::renderer::stage
