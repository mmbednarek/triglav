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
   explicit GenerateCommandListPass(BuildContext& context, PipelineCache& pipeline_cache, DescriptorStorage& descriptor_storage,
                                    ResourceStorage& resource_storage, graphics_api::CommandList& command_list,
                                    graphics_api::DescriptorPool* descriptor_pool, u32 frame_index);

   void visit(const detail::cmd::BindGraphicsPipeline& cmd);
   void visit(const detail::cmd::BindComputePipeline& cmd);
   void visit(const detail::cmd::BindRayTracingPipeline& cmd);
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
   void visit(const detail::cmd::CopyTextureRegion& cmd) const;
   void visit(const detail::cmd::BlitTexture& cmd) const;
   void visit(const detail::cmd::PlaceTextureBarrier& cmd) const;
   void visit(const detail::cmd::PlaceBufferBarrier& cmd) const;
   void visit(const detail::cmd::FillBuffer& cmd) const;
   void visit(const detail::cmd::BeginRenderPass& cmd) const;
   void visit(const detail::cmd::EndRenderPass& cmd) const;
   void visit(const detail::cmd::PushConstant& cmd) const;
   void visit(const detail::cmd::TraceRays& cmd) const;
   void visit(const detail::cmd::ResetQueries& cmd) const;
   void visit(const detail::cmd::QueryTimestamp& cmd) const;
   void visit(const detail::cmd::BeginQuery& cmd) const;
   void visit(const detail::cmd::EndQuery& cmd) const;
   void visit(const detail::cmd::SetViewport& cmd) const;

   void default_visit(const detail::Command&) const;

 private:
   BuildContext& m_context;
   PipelineCache& m_pipeline_cache;
   DescriptorStorage& m_descriptor_storage;
   ResourceStorage& m_resource_storage;
   graphics_api::CommandList& m_command_list;
   graphics_api::Pipeline* m_current_pipeline{nullptr};
   graphics_api::DescriptorPool* m_descriptor_pool{nullptr};
   u32 m_frame_index;
};

}// namespace triglav::render_core
