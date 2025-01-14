#include "GenerateCommandListPass.hpp"

#include "BuildContext.hpp"
#include "PipelineCache.hpp"
#include "ResourceStorage.hpp"

namespace triglav::render_core {

namespace gapi = graphics_api;

GenerateCommandListPass::GenerateCommandListPass(BuildContext& context, PipelineCache& pipelineCache, DescriptorStorage& descriptorStorage,
                                                 ResourceStorage& resourceStorage, graphics_api::CommandList& commandList,
                                                 graphics_api::DescriptorPool* descriptorPool, const u32 frameIndex) :
    m_context(context),
    m_pipelineCache(pipelineCache),
    m_descriptorStorage(descriptorStorage),
    m_resourceStorage(resourceStorage),
    m_commandList(commandList),
    m_descriptorPool(descriptorPool),
    m_frameIndex(frameIndex)
{
}

void GenerateCommandListPass::visit(const detail::cmd::BindGraphicsPipeline& cmd)
{
   auto* newPipeline = &m_pipelineCache.get_graphics_pipeline(cmd.pso);
   if (newPipeline != m_currentPipeline) {
      m_currentPipeline = newPipeline;
      m_commandList.bind_pipeline(*m_currentPipeline);
   }
}

void GenerateCommandListPass::visit(const detail::cmd::BindComputePipeline& cmd)
{
   auto* newPipeline = &m_pipelineCache.get_compute_pipeline(cmd.pso);
   if (newPipeline != m_currentPipeline) {
      m_currentPipeline = newPipeline;
      m_commandList.bind_pipeline(*m_currentPipeline);
   }
}

void GenerateCommandListPass::visit(const detail::cmd::DrawPrimitives& cmd) const
{
   m_commandList.draw_primitives(cmd.vertexCount, cmd.vertexOffset, cmd.instanceCount, cmd.instanceOffset);
}

void GenerateCommandListPass::visit(const detail::cmd::DrawIndexedPrimitives& cmd) const
{
   m_commandList.draw_indexed_primitives(cmd.indexCount, cmd.indexOffset, cmd.vertexOffset, cmd.instanceCount, cmd.instanceOffset);
}

void GenerateCommandListPass::visit(const detail::cmd::DrawIndirectWithCount& cmd) const
{
   m_commandList.draw_indirect_with_count(m_context.resolve_buffer_ref(m_resourceStorage, cmd.drawCallBuffer, m_frameIndex),
                                          m_context.resolve_buffer_ref(m_resourceStorage, cmd.countBuffer, m_frameIndex), cmd.maxDrawCalls,
                                          cmd.stride);
}
void GenerateCommandListPass::visit(const detail::cmd::Dispatch& cmd) const
{
   m_commandList.dispatch(cmd.dims.x, cmd.dims.y, cmd.dims.z);
}

void GenerateCommandListPass::visit(const detail::cmd::BindDescriptors& cmd) const
{
   assert(cmd.descriptors.size() <= MAX_DESCRIPTOR_COUNT);
   assert(m_descriptorPool != nullptr);
   assert(m_currentPipeline != nullptr);

   auto& descriptorArray = m_context.allocate_descriptors(m_descriptorStorage, *m_descriptorPool, *m_currentPipeline);
   m_context.write_descriptor(m_resourceStorage, descriptorArray[0], cmd, m_frameIndex);
   m_commandList.bind_descriptor_set(m_currentPipeline->pipeline_type(), descriptorArray[0]);
}

void GenerateCommandListPass::visit(const detail::cmd::BindVertexBuffer& cmd) const
{
   m_commandList.bind_vertex_buffer(m_context.resolve_buffer_ref(m_resourceStorage, cmd.buffRef, m_frameIndex), 0);
}

void GenerateCommandListPass::visit(const detail::cmd::BindIndexBuffer& cmd) const
{
   m_commandList.bind_index_buffer(m_context.resolve_buffer_ref(m_resourceStorage, cmd.buffRef, m_frameIndex));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyTextureToBuffer& cmd) const
{
   m_commandList.copy_texture_to_buffer(m_context.resolve_texture_ref(m_resourceStorage, cmd.srcTexture, m_frameIndex),
                                        m_context.resolve_buffer_ref(m_resourceStorage, cmd.dstBuffer, m_frameIndex));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyBufferToTexture& cmd) const
{
   m_commandList.copy_buffer_to_texture(m_context.resolve_buffer_ref(m_resourceStorage, cmd.srcBuffer, m_frameIndex),
                                        m_context.resolve_texture_ref(m_resourceStorage, cmd.dstTexture, m_frameIndex));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyBuffer& cmd) const
{
   m_commandList.copy_buffer(m_context.resolve_buffer_ref(m_resourceStorage, cmd.srcBuffer, m_frameIndex),
                             m_context.resolve_buffer_ref(m_resourceStorage, cmd.dstBuffer, m_frameIndex));
}

void GenerateCommandListPass::visit(const detail::cmd::CopyTexture& cmd) const
{
   m_commandList.copy_texture(
      m_context.resolve_texture_ref(m_resourceStorage, cmd.srcTexture, m_frameIndex), graphics_api::TextureState::TransferSrc,
      m_context.resolve_texture_ref(m_resourceStorage, cmd.dstTexture, m_frameIndex), graphics_api::TextureState::TransferDst);
}

void GenerateCommandListPass::visit(const detail::cmd::PlaceTextureBarrier& cmd) const
{
   graphics_api::TextureBarrierInfo info{};
   info.texture = &m_context.resolve_texture_ref(m_resourceStorage, cmd.barrier->textureRef, m_frameIndex);
   info.sourceState = cmd.barrier->srcState;
   info.targetState = cmd.barrier->dstState;
   info.baseMipLevel = cmd.barrier->baseMipLevel;
   info.mipLevelCount = cmd.barrier->mipLevelCount;
   m_commandList.texture_barrier(cmd.barrier->srcStageFlags, cmd.barrier->dstStageFlags, info);
}

void GenerateCommandListPass::visit(const detail::cmd::PlaceBufferBarrier& cmd) const
{
   gapi::BufferBarrier barrier{};
   barrier.buffer = &m_context.resolve_buffer_ref(m_resourceStorage, cmd.barrier->bufferRef, m_frameIndex);
   barrier.srcAccess = cmd.barrier->srcBufferAccess;
   barrier.dstAccess = cmd.barrier->dstBufferAccess;
   m_commandList.buffer_barrier(cmd.barrier->srcStageFlags, cmd.barrier->dstStageFlags, std::array{barrier});
}

void GenerateCommandListPass::visit(const detail::cmd::FillBuffer& cmd) const
{
   m_commandList.update_buffer(m_resourceStorage.buffer(cmd.buffName, m_frameIndex), 0, cmd.data.size(), cmd.data.data());
}

void GenerateCommandListPass::visit(const detail::cmd::BeginRenderPass& cmd) const
{
   auto renderingInfo = m_context.create_rendering_info(m_resourceStorage, cmd, m_frameIndex);
   m_commandList.begin_rendering(renderingInfo);
}

void GenerateCommandListPass::visit(const detail::cmd::EndRenderPass& /*cmd*/) const
{
   m_commandList.end_rendering();
}

void GenerateCommandListPass::visit(const detail::cmd::PushConstant& cmd) const
{
   m_commandList.push_constant_ptr(cmd.stageFlags, cmd.data.data(), cmd.data.size(), 0);
}

void GenerateCommandListPass::default_visit(const detail::Command&) const
{
   assert(false && "unsupported instructions");
}

}// namespace triglav::render_core
