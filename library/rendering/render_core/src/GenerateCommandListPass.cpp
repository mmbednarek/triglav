#include "GenerateCommandListPass.hpp"

#include "BuildContext.hpp"
#include "PipelineCache.hpp"
#include "ResourceStorage.hpp"

#include "triglav/graphics_api/ray_tracing/RayTracingPipeline.hpp"

namespace triglav::render_core {

namespace gapi = graphics_api;

GenerateCommandListPass::GenerateCommandListPass(BuildContext& context, PipelineCache& pipeline_cache,
                                                 DescriptorStorage& descriptor_storage, ResourceStorage& resource_storage,
                                                 graphics_api::CommandList& command_list, graphics_api::DescriptorPool* descriptor_pool,
                                                 const u32 frame_index) :
    m_context(context),
    m_pipeline_cache(pipeline_cache),
    m_descriptor_storage(descriptor_storage),
    m_resource_storage(resource_storage),
    m_command_list(command_list),
    m_descriptor_pool(descriptor_pool),
    m_frame_index(frame_index)
{
}

void GenerateCommandListPass::visit(const detail::cmd::BindGraphicsPipeline& cmd)
{
   auto* new_pipeline = &m_pipeline_cache.get_graphics_pipeline(cmd.pso);
   if (new_pipeline != m_current_pipeline) {
      m_current_pipeline = new_pipeline;
      m_command_list.bind_pipeline(*m_current_pipeline);
   }
}

void GenerateCommandListPass::visit(const detail::cmd::BindComputePipeline& cmd)
{
   auto* new_pipeline = &m_pipeline_cache.get_compute_pipeline(cmd.pso);
   if (new_pipeline != m_current_pipeline) {
      m_current_pipeline = new_pipeline;
      m_command_list.bind_pipeline(*m_current_pipeline);
   }
}

void GenerateCommandListPass::visit(const detail::cmd::BindRayTracingPipeline& cmd)
{
   auto* new_pipeline = &m_pipeline_cache.get_ray_tracing_pso(cmd.pso);
   if (new_pipeline != m_current_pipeline) {
      m_current_pipeline = new_pipeline;
      m_command_list.bind_pipeline(*m_current_pipeline);
   }
}

void GenerateCommandListPass::visit(const detail::cmd::DrawPrimitives& cmd) const
{
   m_command_list.draw_primitives(cmd.vertex_count, cmd.vertex_offset, cmd.instance_count, cmd.instance_offset);
}

void GenerateCommandListPass::visit(const detail::cmd::DrawIndexedPrimitives& cmd) const
{
   m_command_list.draw_indexed_primitives(cmd.index_count, cmd.index_offset, cmd.vertex_offset, cmd.instance_count, cmd.instance_offset);
}

void GenerateCommandListPass::visit(const detail::cmd::DrawIndexedIndirectWithCount& cmd) const
{
   m_command_list.draw_indexed_indirect_with_count(m_context.resolve_buffer_ref(m_resource_storage, cmd.draw_call_buffer, m_frame_index),
                                                   m_context.resolve_buffer_ref(m_resource_storage, cmd.count_buffer, m_frame_index),
                                                   cmd.max_draw_calls, cmd.stride, cmd.count_buffer_offset);
}

void GenerateCommandListPass::visit(const detail::cmd::DrawIndirectWithCount& cmd) const
{
   m_command_list.draw_indirect_with_count(m_context.resolve_buffer_ref(m_resource_storage, cmd.draw_call_buffer, m_frame_index),
                                           m_context.resolve_buffer_ref(m_resource_storage, cmd.count_buffer, m_frame_index),
                                           cmd.max_draw_calls, cmd.stride);
}

void GenerateCommandListPass::visit(const detail::cmd::Dispatch& cmd) const
{
   m_command_list.dispatch(cmd.dims.x, cmd.dims.y, cmd.dims.z);
}

void GenerateCommandListPass::visit(const detail::cmd::DispatchIndirect& cmd) const
{
   m_command_list.dispatch_indirect(m_context.resolve_buffer_ref(m_resource_storage, cmd.indirect_buffer, m_frame_index));
}

void GenerateCommandListPass::visit(const detail::cmd::BindDescriptors& cmd) const
{
   assert(cmd.descriptors.size() <= MAX_DESCRIPTOR_COUNT);
   assert(m_descriptor_pool != nullptr);
   assert(m_current_pipeline != nullptr);

   auto& descriptor_array = m_context.allocate_descriptors(m_descriptor_storage, *m_descriptor_pool, *m_current_pipeline);
   m_context.write_descriptor(m_resource_storage, descriptor_array[0], cmd, m_frame_index);
   m_command_list.bind_descriptor_set(m_current_pipeline->pipeline_type(), descriptor_array[0]);
}

void GenerateCommandListPass::visit(const detail::cmd::BindVertexBuffer& cmd) const
{
   m_command_list.bind_vertex_buffer(m_context.resolve_buffer_ref(m_resource_storage, cmd.buff_ref, m_frame_index), 0);
}

void GenerateCommandListPass::visit(const detail::cmd::BindIndexBuffer& cmd) const
{
   m_command_list.bind_index_buffer(m_context.resolve_buffer_ref(m_resource_storage, cmd.buff_ref, m_frame_index));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyTextureToBuffer& cmd) const
{
   m_command_list.copy_texture_to_buffer(m_context.resolve_texture_ref(m_resource_storage, cmd.src_texture, m_frame_index),
                                         m_context.resolve_buffer_ref(m_resource_storage, cmd.dst_buffer, m_frame_index));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyBufferToTexture& cmd) const
{
   m_command_list.copy_buffer_to_texture(m_context.resolve_buffer_ref(m_resource_storage, cmd.src_buffer, m_frame_index),
                                         m_context.resolve_texture_ref(m_resource_storage, cmd.dst_texture, m_frame_index));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyBuffer& cmd) const
{
   m_command_list.copy_buffer(m_context.resolve_buffer_ref(m_resource_storage, cmd.src_buffer, m_frame_index),
                              m_context.resolve_buffer_ref(m_resource_storage, cmd.dst_buffer, m_frame_index));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyTexture& cmd) const
{
   m_command_list.copy_texture(
      m_context.resolve_texture_ref(m_resource_storage, cmd.src_texture, m_frame_index), graphics_api::TextureState::TransferSrc,
      m_context.resolve_texture_ref(m_resource_storage, cmd.dst_texture, m_frame_index), graphics_api::TextureState::TransferDst);
}

void GenerateCommandListPass::visit(const detail::cmd::CopyTextureRegion& cmd) const
{
   m_command_list.copy_texture_region(m_context.resolve_texture_ref(m_resource_storage, cmd.src_texture, m_frame_index),
                                      graphics_api::TextureState::TransferSrc, cmd.src_offset,
                                      m_context.resolve_texture_ref(m_resource_storage, cmd.dst_texture, m_frame_index),
                                      graphics_api::TextureState::TransferDst, cmd.dst_offset, cmd.size);
}

void GenerateCommandListPass::visit(const detail::cmd::BlitTexture& cmd) const
{
   auto& src_tex = m_context.resolve_texture_ref(m_resource_storage, cmd.src_texture, m_frame_index);
   auto& dst_tex = m_context.resolve_texture_ref(m_resource_storage, cmd.dst_texture, m_frame_index);
   m_command_list.blit_texture(src_tex,
                               graphics_api::TextureRegion{
                                  .offset_min = {0, 0},
                                  .offset_max = {src_tex.resolution().width, src_tex.resolution().height},
                                  .mip_level = 0,
                               },
                               dst_tex,
                               graphics_api::TextureRegion{
                                  .offset_min = {0, 0},
                                  .offset_max = {dst_tex.resolution().width, dst_tex.resolution().height},
                                  .mip_level = 0,
                               });
}

void GenerateCommandListPass::visit(const detail::cmd::PlaceTextureBarrier& cmd) const
{
   graphics_api::TextureBarrierInfo info{};
   info.texture = &m_context.resolve_texture_ref(m_resource_storage, cmd.barrier->texture_ref, m_frame_index);
   info.source_state = cmd.barrier->src_state;
   info.target_state = cmd.barrier->dst_state;
   info.base_mip_level = cmd.barrier->base_mip_level;
   info.mip_level_count = cmd.barrier->mip_level_count;
   m_command_list.texture_barrier(cmd.barrier->src_stage_flags, cmd.barrier->dst_stage_flags, info);
}

void GenerateCommandListPass::visit(const detail::cmd::PlaceBufferBarrier& cmd) const
{
   gapi::BufferBarrier barrier{};
   barrier.buffer = &m_context.resolve_buffer_ref(m_resource_storage, cmd.barrier->buffer_ref, m_frame_index);
   barrier.src_access = cmd.barrier->src_buffer_access;
   barrier.dst_access = cmd.barrier->dst_buffer_access;
   m_command_list.buffer_barrier(cmd.barrier->src_stage_flags, cmd.barrier->dst_stage_flags, std::array{barrier});
}

void GenerateCommandListPass::visit(const detail::cmd::FillBuffer& cmd) const
{
   m_command_list.update_buffer(m_resource_storage.buffer(cmd.buff_name, m_frame_index), 0, static_cast<u32>(cmd.data.size()),
                                cmd.data.data());
}

void GenerateCommandListPass::visit(const detail::cmd::BeginRenderPass& cmd) const
{
   auto rendering_info = m_context.create_rendering_info(m_resource_storage, cmd, m_frame_index);
   m_command_list.begin_rendering(rendering_info);
}

void GenerateCommandListPass::visit(const detail::cmd::EndRenderPass& /*cmd*/) const
{
   m_command_list.end_rendering();
}

void GenerateCommandListPass::visit(const detail::cmd::PushConstant& cmd) const
{
   m_command_list.push_constant_ptr(cmd.stage_flags, cmd.data.data(), cmd.data.size(), 0);
}

void GenerateCommandListPass::visit(const detail::cmd::TraceRays& cmd) const
{
   m_command_list.trace_rays(m_pipeline_cache.get_shader_binding_table(cmd.state), cmd.dimensions);
}

void GenerateCommandListPass::visit(const detail::cmd::ResetQueries& cmd) const
{
   if (cmd.should_reset_timestamp_query_pool) {
      m_command_list.reset_timestamp_array(m_resource_storage.timestamps(), cmd.offset, cmd.count);
   } else {
      m_command_list.reset_timestamp_array(m_resource_storage.pipeline_stats(), cmd.offset, cmd.count);
   }
}

void GenerateCommandListPass::visit(const detail::cmd::QueryTimestamp& cmd) const
{
   m_command_list.write_timestamp(cmd.is_closing ? gapi::PipelineStage::End : gapi::PipelineStage::Entrypoint,
                                  m_resource_storage.timestamps(), cmd.index);
}

void GenerateCommandListPass::visit(const detail::cmd::BeginQuery& cmd) const
{
   m_command_list.begin_query(m_resource_storage.pipeline_stats(), cmd.index);
}

void GenerateCommandListPass::visit(const detail::cmd::EndQuery& cmd) const
{
   m_command_list.end_query(m_resource_storage.pipeline_stats(), cmd.index);
}

void GenerateCommandListPass::visit(const detail::cmd::SetViewport& cmd) const
{
   m_command_list.set_viewport(cmd.dimensions, cmd.min_depth, cmd.max_depth);
}

void GenerateCommandListPass::default_visit(const detail::Command&) const
{
   assert(false && "unsupported instructions");
}

}// namespace triglav::render_core
