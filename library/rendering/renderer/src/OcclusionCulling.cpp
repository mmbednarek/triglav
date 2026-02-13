#include "OcclusionCulling.hpp"

#include "BindlessScene.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer {

constexpr auto g_draw_call_size = 124;
constexpr auto g_bindless_object_limit = 80;

using namespace name_literals;
using namespace render_core::literals;

OcclusionCulling::OcclusionCulling(UpdateViewParamsJob& update_view_job, BindlessScene& bindless_scene) :
    m_bindless_scene(bindless_scene),
    TG_CONNECT(update_view_job, OnResourceDefinition, on_resource_definition),
    TG_CONNECT(update_view_job, OnViewPropertiesChanged, on_view_properties_changed),
    TG_CONNECT(update_view_job, OnViewPropertiesNotChanged, on_view_properties_not_changed),
    TG_CONNECT(update_view_job, OnFinalize, on_finalize)
{
}

void OcclusionCulling::on_resource_definition(render_core::BuildContext& ctx) const
{
   // buffers
   ctx.declare_buffer("occlusion_culling.count_buffer"_name, 3 * sizeof(u32));
   ctx.declare_proportional_texture("occlusion_culling.hierarchical_depth_buffer"_name, GAPI_FORMAT(R, UNorm16), 0.5f, true);
   ctx.declare_proportional_buffer("occlusion_culling.staging_depth_buffer"_name, 0.5f, sizeof(u16));
   ctx.declare_buffer("occlusion_culling.visible_objects.mt0"_name, sizeof(DrawCall) * g_bindless_object_limit);
   ctx.declare_buffer("occlusion_culling.visible_objects.mt1"_name, sizeof(DrawCall) * g_bindless_object_limit);
   ctx.declare_buffer("occlusion_culling.visible_objects.mt2"_name, sizeof(DrawCall) * g_bindless_object_limit);
   ctx.declare_buffer("occlusion_culling.passthrough"_name, sizeof(DrawCall) * g_bindless_object_limit * 3);

   // rts
   ctx.declare_proportional_depth_target("occlusion_culling.depth_prepass_target"_name, GAPI_FORMAT(D, UNorm16), 0.5f);
}

void OcclusionCulling::on_view_properties_changed(render_core::BuildContext& ctx) const
{
   // Render Depth Pre-Pass
   {
      render_core::RenderPassScope rt_scope(ctx, "occlusion_culling.depth_prepass"_name, "occlusion_culling.depth_prepass_target"_name);
      ctx.clear_depth_stencil("occlusion_culling.depth_prepass_target"_name, 1.0f, 0);

      this->draw_pre_pass_objects(ctx, "occlusion_culling.visible_objects.mt0"_last_frame, "occlusion_culling.count_buffer"_last_frame, 0);
      this->draw_pre_pass_objects(ctx, "occlusion_culling.visible_objects.mt1"_last_frame, "occlusion_culling.count_buffer"_last_frame, 1);
      this->draw_pre_pass_objects(ctx, "occlusion_culling.visible_objects.mt2"_last_frame, "occlusion_culling.count_buffer"_last_frame, 2);
   }

   // Copy depth buffer to the highest mip of Hi-Z

   ctx.copy_texture_to_buffer("occlusion_culling.depth_prepass_target"_name, "occlusion_culling.staging_depth_buffer"_name);
   ctx.copy_buffer_to_texture("occlusion_culling.staging_depth_buffer"_name, "occlusion_culling.hierarchical_depth_buffer"_name);

   // Generate the hierarchy

   const u32 mip_count = render_core::calculate_mip_count(ctx.screen_size() / 2);
   int depth_width = ctx.screen_size().x / 4;
   int depth_height = ctx.screen_size().y / 4;

   for (const u32 mip_index : Range(0u, mip_count - 1)) {
      ctx.bind_compute_shader("shader/bindless_geometry/hi_zbuffer_construct.cshader"_rc);

      ctx.bind_texture(0, render_core::TextureMip{"occlusion_culling.hierarchical_depth_buffer"_name, mip_index});
      ctx.bind_rw_texture(1, render_core::TextureMip{"occlusion_culling.hierarchical_depth_buffer"_name, mip_index + 1});

      ctx.dispatch({divide_rounded_up(depth_width, 32), divide_rounded_up(depth_height, 32), 1});

      depth_width = std::max(depth_width / 2, 1);
      depth_height = std::max(depth_height / 2, 1);
   }

   // Reset the count and cull the objects

   ctx.fill_buffer("occlusion_culling.count_buffer"_name, std::array<u32, 3>{0, 0, 0});

   ctx.bind_compute_shader("shader/bindless_geometry/culling.cshader"_rc);

   ctx.bind_storage_buffer(0, &m_bindless_scene.scene_object_buffer());
   ctx.bind_uniform_buffer(1, &m_bindless_scene.count_buffer());
   ctx.bind_storage_buffer(2, "occlusion_culling.count_buffer"_name);
   ctx.bind_uniform_buffer(3, "core.view_properties"_name);
   ctx.bind_texture(4, "occlusion_culling.hierarchical_depth_buffer"_name);

   ctx.bind_storage_buffer(5, "occlusion_culling.visible_objects.mt0"_name);
   ctx.bind_storage_buffer(6, "occlusion_culling.visible_objects.mt1"_name);
   ctx.bind_storage_buffer(7, "occlusion_culling.visible_objects.mt2"_name);

   ctx.dispatch({divide_rounded_up(m_bindless_scene.scene_object_count(), 1024), 1, 1});

   // Passthrough

   ctx.bind_compute_shader("shader/bindless_geometry/passthrough.cshader"_rc);

   ctx.bind_storage_buffer(0, &m_bindless_scene.scene_object_buffer());
   ctx.bind_uniform_buffer(1, &m_bindless_scene.count_buffer());
   ctx.bind_storage_buffer(2, "occlusion_culling.passthrough"_name);

   ctx.dispatch({divide_rounded_up(m_bindless_scene.scene_object_count(), 256), 1, 1});
}

void OcclusionCulling::on_view_properties_not_changed(render_core::BuildContext& ctx) const
{
   // Just copy visible objects from the previous frame
   ctx.copy_buffer("occlusion_culling.visible_objects.mt0"_last_frame, "occlusion_culling.visible_objects.mt0"_name);
   ctx.copy_buffer("occlusion_culling.visible_objects.mt1"_last_frame, "occlusion_culling.visible_objects.mt1"_name);
   ctx.copy_buffer("occlusion_culling.visible_objects.mt2"_last_frame, "occlusion_culling.visible_objects.mt2"_name);
   ctx.copy_buffer("occlusion_culling.count_buffer"_last_frame, "occlusion_culling.count_buffer"_name);
   ctx.copy_buffer("occlusion_culling.passthrough"_last_frame, "occlusion_culling.passthrough"_name);
}

void OcclusionCulling::on_finalize(render_core::BuildContext& ctx) const
{
   ctx.export_buffer("occlusion_culling.visible_objects.mt0"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
   ctx.export_buffer("occlusion_culling.visible_objects.mt1"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
   ctx.export_buffer("occlusion_culling.visible_objects.mt2"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
   ctx.export_buffer("occlusion_culling.count_buffer"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
   ctx.export_buffer("occlusion_culling.passthrough"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
}

void OcclusionCulling::draw_pre_pass_objects(render_core::BuildContext& ctx, const render_core::BufferRef object_buffer,
                                             const render_core::BufferRef count_buffer, const u32 index) const
{
   ctx.bind_vertex_shader("shader/bindless_geometry/depth_prepass.vshader"_rc);

   render_core::VertexLayout layout(sizeof(geometry::Vertex));
   layout.add("position"_name, GAPI_FORMAT(RGBA, Float32), 0);
   ctx.bind_vertex_layout(layout);

   ctx.bind_uniform_buffer(0, "core.view_properties"_name);
   ctx.bind_storage_buffer(1, object_buffer);

   ctx.bind_fragment_shader("shader/bindless_geometry/depth_prepass.fshader"_rc);

   ctx.bind_vertex_buffer(&m_bindless_scene.combined_vertex_buffer());
   ctx.bind_index_buffer(&m_bindless_scene.combined_index_buffer());

   ctx.draw_indexed_indirect_with_count(object_buffer, count_buffer, g_bindless_object_limit, sizeof(DrawCall), sizeof(u32) * index);
}

void OcclusionCulling::reset_buffers(graphics_api::Device& device, render_core::JobGraph& graph)
{
   static constexpr std::array<u32, 3> zero_counts{0, 0, 0};

   const auto cmd_list = GAPI_CHECK(device.create_command_list(graphics_api::WorkType::Transfer));

   GAPI_CHECK_STATUS(cmd_list.begin(graphics_api::SubmitType::OneTime));
   cmd_list.update_buffer(graph.resources().buffer("occlusion_culling.count_buffer"_name, 0), 0,
                          static_cast<u32>(zero_counts.size() * sizeof(u32)), zero_counts.data());
   cmd_list.update_buffer(graph.resources().buffer("occlusion_culling.count_buffer"_name, 1), 0,
                          static_cast<u32>(zero_counts.size() * sizeof(u32)), zero_counts.data());
   cmd_list.update_buffer(graph.resources().buffer("occlusion_culling.count_buffer"_name, 2), 0,
                          static_cast<u32>(zero_counts.size() * sizeof(u32)), zero_counts.data());
   GAPI_CHECK_STATUS(cmd_list.finish());

   GAPI_CHECK_STATUS(device.submit_command_list_one_time(cmd_list));
}

}// namespace triglav::renderer