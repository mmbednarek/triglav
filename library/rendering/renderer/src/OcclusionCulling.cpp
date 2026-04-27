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
   ctx.declare_buffer("occlusion_culling.count_buffer"_name, render_objects::GEOMETRY_RENDER_INFOS.size() * sizeof(u32));
   ctx.declare_proportional_texture("occlusion_culling.hierarchical_depth_buffer"_name, GAPI_FORMAT(R, UNorm16), 0.5f, true);
   ctx.declare_proportional_buffer("occlusion_culling.staging_depth_buffer"_name, 0.5f, sizeof(u16));
   ctx.declare_buffer("occlusion_culling.passthrough.count_buffer"_name, render_objects::VERTEX_LAYOUT_INFOS.size() * sizeof(u32));

   for (const auto& render_info : render_objects::GEOMETRY_RENDER_INFOS) {
      ctx.declare_buffer(render_info.draw_call_buffer, sizeof(DrawCall) * g_bindless_object_limit);
   }

   for (const auto& vertex_info : render_objects::VERTEX_LAYOUT_INFOS) {
      ctx.declare_buffer(vertex_info.passthrough_buffer, sizeof(DrawCall) * g_bindless_object_limit * 3);
   }

   // rts
   ctx.declare_proportional_depth_target("occlusion_culling.depth_prepass_target"_name, GAPI_FORMAT(D, UNorm16), 0.5f);
}

void OcclusionCulling::on_view_properties_changed(render_core::BuildContext& ctx) const
{
   TG_DEBUG_LABEL(ctx, "Occlusion culling", {0.2f, 0.2f, 0.8f, 1.0f})

   // Render Depth Pre-Pass
   {
      TG_DEBUG_LABEL(ctx, "Depth pre pass", {0.8f, 0.2f, 0.2f, 1.0f})

      render_core::RenderPassScope rt_scope(ctx, "occlusion_culling.depth_prepass"_name, "occlusion_culling.depth_prepass_target"_name);
      ctx.clear_depth_stencil("occlusion_culling.depth_prepass_target"_name, 1.0f, 0);

      for (const auto& info : render_objects::GEOMETRY_RENDER_INFOS) {
         this->draw_pre_pass_objects(ctx, info);
      }
   }

   {
      TG_DEBUG_LABEL(ctx, "Pre-pass texture to hi-z bottom", {0.8f, 0.2f, 0.2f, 1.0f})

      // Copy depth buffer to the highest mip of Hi-Z
      ctx.copy_texture_to_buffer("occlusion_culling.depth_prepass_target"_name, "occlusion_culling.staging_depth_buffer"_name);
      ctx.copy_buffer_to_texture("occlusion_culling.staging_depth_buffer"_name, "occlusion_culling.hierarchical_depth_buffer"_name);
   }

   {
      TG_DEBUG_LABEL(ctx, "Generate hi-Z", {0.8f, 0.2f, 0.2f, 1.0f})

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
   }

   {
      TG_DEBUG_LABEL(ctx, "Generate transform matrices", {0.8f, 0.2f, 0.2f, 1.0f})
      ctx.bind_compute_shader("shader/bindless_geometry/transform_to_matrix.cshader"_rc);

      ctx.bind_storage_buffer(0, &m_bindless_scene.transform_buffer());
      ctx.bind_storage_buffer(1, &m_bindless_scene.transform_matrix_buffer());
      ctx.bind_uniform_buffer(2, &m_bindless_scene.transform_offset_count_buffer());

      ctx.dispatch({divide_rounded_up(m_bindless_scene.transform_allocated_area().size, 256), 1, 1});
   }

   {
      TG_DEBUG_LABEL(ctx, "Multiply matrix hierarchy", {0.8f, 0.2f, 0.2f, 1.0f})
      ctx.bind_compute_shader("shader/bindless_geometry/matrix_multiply.cshader"_rc);

      ctx.bind_storage_buffer(0, &m_bindless_scene.matrix_hierarchy_buffer());
      ctx.bind_uniform_buffer(1, &m_bindless_scene.matrix_hierarchy_count_buffer());
      ctx.bind_storage_buffer(2, &m_bindless_scene.transform_matrix_buffer());

      ctx.dispatch({std::max(divide_rounded_up(m_bindless_scene.matrix_hierarchy_count(), 256), 1u), 1, 1});
   }

   // Reset the count and cull the objects
   {
      TG_DEBUG_LABEL(ctx, "Cull objects", {0.8f, 0.2f, 0.2f, 1.0f})
      ctx.fill_buffer("occlusion_culling.count_buffer"_name, std::array<u32, render_objects::GEOMETRY_RENDER_INFOS.size()>{});

      ctx.bind_compute_shader("shader/bindless_geometry/culling.cshader"_rc);

      ctx.bind_storage_buffer(0, &m_bindless_scene.scene_object_buffer());
      ctx.bind_uniform_buffer(1, &m_bindless_scene.count_buffer());
      ctx.bind_storage_buffer(2, &m_bindless_scene.transform_matrix_buffer());
      ctx.bind_storage_buffer(3, "occlusion_culling.count_buffer"_name);
      ctx.bind_uniform_buffer(4, "core.view_properties"_name);
      ctx.bind_texture(5, "occlusion_culling.hierarchical_depth_buffer"_name);

      u32 descriptor_index = 6;
      for (const auto& info : render_objects::GEOMETRY_RENDER_INFOS) {
         ctx.bind_storage_buffer(descriptor_index, info.draw_call_buffer);
         ++descriptor_index;
      }

      ctx.dispatch({divide_rounded_up(m_bindless_scene.scene_object_count(), 1024), 1, 1});
   }

   {
      TG_DEBUG_LABEL(ctx, "Cull passthrough", {0.8f, 0.2f, 0.2f, 1.0f})
      ctx.bind_compute_shader("shader/bindless_geometry/passthrough.cshader"_rc);

      ctx.bind_storage_buffer(0, &m_bindless_scene.scene_object_buffer());
      ctx.bind_uniform_buffer(1, &m_bindless_scene.count_buffer());
      ctx.bind_storage_buffer(2, &m_bindless_scene.transform_matrix_buffer());
      ctx.bind_storage_buffer(3, "occlusion_culling.passthrough.count_buffer"_name);

      u32 descriptor_index = 4;
      for (const auto& info : render_objects::VERTEX_LAYOUT_INFOS) {
         ctx.bind_storage_buffer(descriptor_index, info.passthrough_buffer);
         ++descriptor_index;
      }

      ctx.dispatch({divide_rounded_up(m_bindless_scene.scene_object_count(), 256), 1, 1});
   }
}

void OcclusionCulling::on_view_properties_not_changed(render_core::BuildContext& ctx) const
{
   // Just copy visible objects from the previous frame
   for (const auto& info : render_objects::GEOMETRY_RENDER_INFOS) {
      ctx.copy_buffer(render_core::FromLastFrame(info.draw_call_buffer), info.draw_call_buffer);
   }

   for (const auto& info : render_objects::VERTEX_LAYOUT_INFOS) {
      ctx.copy_buffer(render_core::FromLastFrame(info.passthrough_buffer), info.passthrough_buffer);
   }

   ctx.copy_buffer("occlusion_culling.count_buffer"_last_frame, "occlusion_culling.count_buffer"_name);
}

void OcclusionCulling::on_finalize(render_core::BuildContext& ctx) const
{
   for (const auto& info : render_objects::GEOMETRY_RENDER_INFOS) {
      ctx.export_buffer(info.draw_call_buffer, graphics_api::PipelineStage::DrawIndirect, graphics_api::BufferAccess::IndirectCmdRead,
                        graphics_api::BufferUsage::Indirect);
   }

   for (const auto& info : render_objects::VERTEX_LAYOUT_INFOS) {
      ctx.export_buffer(info.passthrough_buffer, graphics_api::PipelineStage::DrawIndirect, graphics_api::BufferAccess::IndirectCmdRead,
                        graphics_api::BufferUsage::Indirect);
   }
   ctx.export_buffer("occlusion_culling.count_buffer"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);

   ctx.export_buffer("occlusion_culling.passthrough.count_buffer"_name, graphics_api::PipelineStage::DrawIndirect,
                     graphics_api::BufferAccess::IndirectCmdRead, graphics_api::BufferUsage::Indirect);
}

void OcclusionCulling::draw_pre_pass_objects(render_core::BuildContext& ctx, const render_objects::MaterialGeometryRenderInfo& info) const
{
   const auto& vertex_info = render_objects::VERTEX_LAYOUT_INFOS[info.vertex_layout_id];

   if (vertex_info.components & geometry::VertexComponent::Skeleton) {
      // TODO: Support skeletal mesh
      return;
   }

   ctx.bind_vertex_shader("shader/bindless_geometry/depth_prepass.vshader"_rc);

   const auto vertex_layout = render_core::vertex_layout_from_components_for_depth_only(vertex_info.components);
   ctx.bind_vertex_layout(vertex_layout);

   ctx.bind_uniform_buffer(0, "core.view_properties"_name);
   ctx.bind_storage_buffer(1, info.draw_call_buffer);

   ctx.bind_fragment_shader("shader/bindless_geometry/depth_prepass.fshader"_rc);

   ctx.bind_vertex_buffer(&m_bindless_scene.combined_vertex_buffer());
   ctx.bind_index_buffer(&m_bindless_scene.combined_index_buffer());

   ctx.draw_indexed_indirect_with_count(info.draw_call_buffer, "occlusion_culling.count_buffer"_last_frame, g_bindless_object_limit,
                                        sizeof(DrawCall), sizeof(u32) * info.index);
}

void OcclusionCulling::reset_buffers(graphics_api::Device& device, render_core::JobGraph& graph)
{
   static constexpr std::array<u32, render_objects::GEOMETRY_RENDER_INFOS.size()> zero_counts{};

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