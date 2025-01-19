#pragma once

#include "detail/Commands.hpp"

#include <cassert>

namespace triglav::graphics_api {
class CommandList;
class Pipeline;
}// namespace triglav::graphics_api

namespace triglav::render_core {

class PipelineCache;
class BuildContext;
class DescriptorStorage;
class ResourceStorage;

class GenerateCommandListPass
{
 public:
   explicit GenerateCommandListPass(BuildContext& context, PipelineCache& pipelineCache, DescriptorStorage& descriptorStorage,
                                    ResourceStorage& resourceStorage, graphics_api::CommandList& commandList,
                                    graphics_api::DescriptorPool* descriptorPool, u32 frameIndex);

   void visit(const detail::cmd::BindGraphicsPipeline& cmd);
   void visit(const detail::cmd::BindComputePipeline& cmd);
   void visit(const detail::cmd::DrawPrimitives& cmd) const;
   void visit(const detail::cmd::DrawIndexedPrimitives& cmd) const;
   void visit(const detail::cmd::DrawIndexedIndirectWithCount& cmd) const;
   void visit(const detail::cmd::DrawIndirectWithCount& cmd) const;
   void visit(const detail::cmd::Dispatch& cmd) const;
   void visit(const detail::cmd::DispatchIndirect& cmd) const;
   void visit(const detail::cmd::BindDescriptors& cmd) const;
   void visit(const detail::cmd::BindVertexBuffer& cmd) const;
   void visit(const detail::cmd::BindIndexBuffer& cmd) const;
   void visit(const detail::cmd::CopyTextureToBuffer& cmd) const;
   void visit(const detail::cmd::CopyBufferToTexture& cmd) const;
   void visit(const detail::cmd::CopyBuffer& cmd) const;
   void visit(const detail::cmd::CopyTexture& cmd) const;
   void visit(const detail::cmd::PlaceTextureBarrier& cmd) const;
   void visit(const detail::cmd::PlaceBufferBarrier& cmd) const;
   void visit(const detail::cmd::FillBuffer& cmd) const;
   void visit(const detail::cmd::BeginRenderPass& cmd) const;
   void visit(const detail::cmd::EndRenderPass& cmd) const;
   void visit(const detail::cmd::PushConstant& cmd) const;

   void default_visit(const detail::Command&) const;

 private:
   BuildContext& m_context;
   PipelineCache& m_pipelineCache;
   DescriptorStorage& m_descriptorStorage;
   ResourceStorage& m_resourceStorage;
   graphics_api::CommandList& m_commandList;
   graphics_api::Pipeline* m_currentPipeline{nullptr};
   graphics_api::DescriptorPool* m_descriptorPool{nullptr};
   u32 m_frameIndex;
};

}// namespace triglav::render_core
