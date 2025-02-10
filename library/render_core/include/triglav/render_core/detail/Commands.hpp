#pragma once

#include "../RenderCore.hpp"
#include "Descriptors.hpp"

namespace triglav::render_core::detail {

namespace cmd {

struct BindGraphicsPipeline
{
   GraphicPipelineState pso;
};

struct BindComputePipeline
{
   ComputePipelineState pso;
};

struct BindRayTracingPipeline
{
   RayTracingPipelineState pso;
};

struct BindDescriptors
{
   std::vector<std::optional<DescriptorAndStage>> descriptors{};
};

struct BindVertexBuffer
{
   BufferRef buffRef{};
};

struct BindIndexBuffer
{
   BufferRef buffRef{};
};

struct PlaceTextureBarrier
{
   std::unique_ptr<TextureBarrier> barrier;

   PlaceTextureBarrier() = default;

   explicit PlaceTextureBarrier(std::unique_ptr<TextureBarrier> barrier) :
       barrier(std::move(barrier))
   {
   }

   PlaceTextureBarrier(PlaceTextureBarrier&&) noexcept = default;
   PlaceTextureBarrier& operator=(PlaceTextureBarrier&&) noexcept = default;

   PlaceTextureBarrier(const PlaceTextureBarrier& other) :
       barrier(std::make_unique<TextureBarrier>(*other.barrier))
   {
   }

   PlaceTextureBarrier& operator=(const PlaceTextureBarrier& other)
   {
      barrier = std::make_unique<TextureBarrier>(*other.barrier);
      return *this;
   }
};

struct PlaceBufferBarrier
{
   std::unique_ptr<BufferBarrier> barrier;

   PlaceBufferBarrier() = default;

   explicit PlaceBufferBarrier(std::unique_ptr<BufferBarrier> barrier) :
       barrier(std::move(barrier))
   {
   }

   ~PlaceBufferBarrier() = default;

   PlaceBufferBarrier(PlaceBufferBarrier&&) noexcept = default;
   PlaceBufferBarrier& operator=(PlaceBufferBarrier&&) noexcept = default;

   PlaceBufferBarrier(const PlaceBufferBarrier& other) :
       barrier(std::make_unique<BufferBarrier>(*other.barrier))
   {
   }

   PlaceBufferBarrier& operator=(const PlaceBufferBarrier& other)
   {
      barrier = std::make_unique<BufferBarrier>(*other.barrier);
      return *this;
   }
};

struct DrawPrimitives
{
   u32 vertexCount{};
   u32 vertexOffset{};
   u32 instanceCount{};
   u32 instanceOffset{};
};

struct DrawIndexedPrimitives
{
   u32 indexCount{};
   u32 indexOffset{};
   u32 vertexOffset{};
   u32 instanceCount{};
   u32 instanceOffset{};
};

struct DrawIndexedIndirectWithCount
{
   BufferRef drawCallBuffer{};
   BufferRef countBuffer{};
   u32 maxDrawCalls{};
   u32 stride{};
};

struct DrawIndirectWithCount
{
   BufferRef drawCallBuffer{};
   BufferRef countBuffer{};
   u32 maxDrawCalls{};
   u32 stride{};
};

struct Dispatch
{
   Vector3i dims;
};

struct DispatchIndirect
{
   BufferRef indirectBuffer{};
};

struct CopyTextureToBuffer
{
   TextureRef srcTexture;
   BufferRef dstBuffer;
};

struct CopyBufferToTexture
{
   BufferRef srcBuffer;
   TextureRef dstTexture;
};

struct CopyBuffer
{
   BufferRef srcBuffer;
   BufferRef dstBuffer;
};

struct CopyTexture
{
   TextureRef srcTexture;
   TextureRef dstTexture;
};

struct FillBuffer
{
   Name buffName{};
   std::vector<u8> data{};
};

struct BeginRenderPass
{
   Name passName;
   std::vector<Name> renderTargets;
};

struct EndRenderPass
{};

struct IfEnabledCond
{
   Name flag;
};

struct IfDisabledCond
{
   Name flag;
};

struct EndIfCond
{};

struct ExportTexture
{
   Name texName;
   graphics_api::PipelineStage pipelineStage;
   graphics_api::TextureState state;
};

struct ExportBuffer
{
   Name buffName;
   graphics_api::PipelineStage pipelineStage;
   graphics_api::BufferAccess access;
};

struct PushConstant
{
   graphics_api::PipelineStageFlags stageFlags;
   std::vector<u8> data;
};

struct TraceRays
{
   RayTracingPipelineState state;
   Vector3i dimensions;
};

struct ResetQueries
{
   u32 offset;
   u32 count;
};

struct QueryTimestamp
{
   u32 index;
   bool isClosing;
};

}// namespace cmd

using Command =
   std::variant<cmd::BindGraphicsPipeline, cmd::BindComputePipeline, cmd::BindRayTracingPipeline, cmd::DrawPrimitives,
                cmd::DrawIndexedPrimitives, cmd::DrawIndexedIndirectWithCount, cmd::DrawIndirectWithCount, cmd::Dispatch,
                cmd::DispatchIndirect, cmd::BindDescriptors, cmd::BindVertexBuffer, cmd::BindIndexBuffer, cmd::CopyTextureToBuffer,
                cmd::CopyBufferToTexture, cmd::CopyBuffer, cmd::CopyTexture, cmd::PlaceTextureBarrier, cmd::PlaceBufferBarrier,
                cmd::FillBuffer, cmd::BeginRenderPass, cmd::EndRenderPass, cmd::IfEnabledCond, cmd::IfDisabledCond, cmd::EndIfCond,
                cmd::ExportTexture, cmd::ExportBuffer, cmd::PushConstant, cmd::TraceRays, cmd::ResetQueries, cmd::QueryTimestamp>;

}// namespace triglav::render_core::detail
