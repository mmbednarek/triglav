#include "OcclusionCulling.hpp"

#include "BindlessScene.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/render_core/BuildContext.hpp"
#include "triglav/render_core/JobGraph.hpp"

namespace triglav::renderer {

constexpr auto g_bindlessObjectLimit = 80;

using namespace name_literals;
using namespace render_core::literals;

OcclusionCulling::OcclusionCulling(UpdateViewParamsJob& updateViewJob, BindlessScene& bindlessScene) :
    m_bindlessScene(bindlessScene),
    TG_CONNECT(updateViewJob, OnResourceDefinition, on_resource_definition),
    TG_CONNECT(updateViewJob, OnViewPropertiesChanged, on_view_properties_changed),
    TG_CONNECT(updateViewJob, OnViewPropertiesNotChanged, on_view_properties_not_changed),
    TG_CONNECT(updateViewJob, OnFinalize, on_finalize)
{
}

void OcclusionCulling::on_resource_definition(render_core::BuildContext& ctx) const
{
   // buffers
   ctx.declare_buffer("occlusion_culling.count_buffer"_name, 3 * sizeof(u32));
   ctx.declare_proportional_texture("occlusion_culling.hierarchical_depth_buffer"_name, GAPI_FORMAT(R, UNorm16), 0.5f, true);
   ctx.declare_proportional_buffer("occlusion_culling.staging_depth_buffer"_name, 0.5f, sizeof(u16));
   ctx.declare_buffer("occlusion_culling.visible_objects.mt0"_name, sizeof(BindlessSceneObject) * g_bindlessObjectLimit);
   ctx.declare_buffer("occlusion_culling.visible_objects.mt1"_name, sizeof(BindlessSceneObject) * g_bindlessObjectLimit);
   ctx.declare_buffer("occlusion_culling.visible_objects.mt2"_name, sizeof(BindlessSceneObject) * g_bindlessObjectLimit);

   // rts
   ctx.declare_proportional_depth_target("occlusion_culling.depth_prepass_target"_name, GAPI_FORMAT(D, UNorm16), 0.5f);
}

void OcclusionCulling::on_view_properties_changed(render_core::BuildContext& ctx) const
{
   // Render Depth Pre-Pass
   {
      render_core::RenderPassScope rtScope(ctx, "occlusion_culling.depth_prepass"_name, "occlusion_culling.depth_prepass_target"_name);

      this->draw_pre_pass_objects(ctx, "occlusion_culling.visible_objects.mt0"_last_frame, "occlusion_culling.count_buffer"_last_frame, 0);
      this->draw_pre_pass_objects(ctx, "occlusion_culling.visible_objects.mt1"_last_frame, "occlusion_culling.count_buffer"_last_frame, 1);
      this->draw_pre_pass_objects(ctx, "occlusion_culling.visible_objects.mt2"_last_frame, "occlusion_culling.count_buffer"_last_frame, 2);
   }

   // Copy depth buffer to the highest mip of Hi-Z

   ctx.copy_texture_to_buffer("occlusion_culling.depth_prepass_target"_name, "occlusion_culling.staging_depth_buffer"_name);
   ctx.copy_buffer_to_texture("occlusion_culling.staging_depth_buffer"_name, "occlusion_culling.hierarchical_depth_buffer"_name);

   // Generate the hierarchy

   const u32 mipCount = render_core::calculate_mip_count(ctx.screen_size() / 2);
   int depthWidth = ctx.screen_size().x / 4;
   int depthHeight = ctx.screen_size().y / 4;

   for (const u32 mipIndex : Range(0u, mipCount - 1)) {
      ctx.bind_compute_shader("bindless_geometry_hi_zbuffer.cshader"_rc);

      ctx.bind_texture(0, render_core::TextureMip{"occlusion_culling.hierarchical_depth_buffer"_name, mipIndex});
      ctx.bind_rw_texture(1, render_core::TextureMip{"occlusion_culling.hierarchical_depth_buffer"_name, mipIndex + 1});

      ctx.dispatch({divide_rounded_up(depthWidth, 32), divide_rounded_up(depthHeight, 32), 1});

      depthWidth = std::max(depthWidth / 2, 1);
      depthHeight = std::max(depthHeight / 2, 1);
   }

   // Reset the count and cull the objects

   ctx.fill_buffer("occlusion_culling.count_buffer"_name, std::array<u32, 3>{0, 0, 0});

   ctx.bind_compute_shader("bindless_geometry_culling.cshader"_rc);

   ctx.bind_storage_buffer(0, &m_bindlessScene.scene_object_buffer());
   ctx.bind_storage_buffer(1, "occlusion_culling.count_buffer"_name);
   ctx.bind_uniform_buffer(2, "core.view_properties"_name);
   ctx.bind_texture(3, "occlusion_culling.hierarchical_depth_buffer"_name);

   ctx.bind_storage_buffer(4, "occlusion_culling.visible_objects.mt0"_name);
   ctx.bind_storage_buffer(5, "occlusion_culling.visible_objects.mt1"_name);
   ctx.bind_storage_buffer(6, "occlusion_culling.visible_objects.mt2"_name);

   ctx.dispatch({divide_rounded_up(m_bindlessScene.scene_object_count(), 1024), 1, 1});
}

void OcclusionCulling::on_view_properties_not_changed(render_core::BuildContext& ctx) const
{
   // Just copy visible objects from previous frame
   ctx.copy_buffer("occlusion_culling.visible_objects.mt0"_last_frame, "occlusion_culling.visible_objects.mt0"_name);
   ctx.copy_buffer("occlusion_culling.visible_objects.mt1"_last_frame, "occlusion_culling.visible_objects.mt1"_name);
   ctx.copy_buffer("occlusion_culling.visible_objects.mt2"_last_frame, "occlusion_culling.visible_objects.mt2"_name);
   ctx.copy_buffer("occlusion_culling.count_buffer"_last_frame, "occlusion_culling.count_buffer"_name);
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
}

void OcclusionCulling::draw_pre_pass_objects(render_core::BuildContext& ctx, const render_core::BufferRef objectBuffer,
                                             const render_core::BufferRef countBuffer, const u32 index) const
{
   ctx.bind_vertex_shader("bindless_geometry_depth_prepass.vshader"_rc);

   render_core::VertexLayout layout(sizeof(geometry::Vertex));
   layout.add("position"_name, GAPI_FORMAT(RGBA, Float32), 0);
   ctx.bind_vertex_layout(layout);

   ctx.bind_uniform_buffer(0, "core.view_properties"_name);
   ctx.bind_storage_buffer(1, objectBuffer);

   ctx.bind_fragment_shader("bindless_geometry_depth_prepass.fshader"_rc);

   ctx.bind_vertex_buffer(&m_bindlessScene.combined_vertex_buffer());
   ctx.bind_index_buffer(&m_bindlessScene.combined_index_buffer());

   ctx.draw_indexed_indirect_with_count(objectBuffer, countBuffer, g_bindlessObjectLimit, sizeof(BindlessSceneObject), sizeof(u32) * index);
}

void OcclusionCulling::reset_buffers(graphics_api::Device& device, render_core::JobGraph& graph)
{
   static constexpr std::array<u32, 3> zeroCounts{0, 0, 0};

   const auto cmdList = GAPI_CHECK(device.create_command_list(graphics_api::WorkType::Transfer));

   GAPI_CHECK_STATUS(cmdList.begin(graphics_api::SubmitType::OneTime));
   cmdList.update_buffer(graph.resources().buffer("occlusion_culling.count_buffer"_name, 0), 0,
                         static_cast<u32>(zeroCounts.size() * sizeof(u32)), zeroCounts.data());
   cmdList.update_buffer(graph.resources().buffer("occlusion_culling.count_buffer"_name, 1), 0,
                         static_cast<u32>(zeroCounts.size() * sizeof(u32)), zeroCounts.data());
   cmdList.update_buffer(graph.resources().buffer("occlusion_culling.count_buffer"_name, 2), 0,
                         static_cast<u32>(zeroCounts.size() * sizeof(u32)), zeroCounts.data());
   GAPI_CHECK_STATUS(cmdList.finish());

   GAPI_CHECK_STATUS(device.submit_command_list_one_time(cmdList));
}

}// namespace triglav::renderer