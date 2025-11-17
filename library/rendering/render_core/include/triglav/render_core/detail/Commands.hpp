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
   BufferRef buff_ref{};
};

struct BindIndexBuffer
{
   BufferRef buff_ref{};
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
   u32 vertex_count{};
   u32 vertex_offset{};
   u32 instance_count{};
   u32 instance_offset{};
};

struct DrawIndexedPrimitives
{
   u32 index_count{};
   u32 index_offset{};
   u32 vertex_offset{};
   u32 instance_count{};
   u32 instance_offset{};
};

struct DrawIndexedIndirectWithCount
{
   BufferRef draw_call_buffer{};
   BufferRef count_buffer{};
   u32 max_draw_calls{};
   u32 stride{};
   u32 count_buffer_offset{};
};

struct DrawIndirectWithCount
{
   BufferRef draw_call_buffer{};
   BufferRef count_buffer{};
   u32 max_draw_calls{};
   u32 stride{};
};

struct Dispatch
{
   Vector3i dims;
};

struct DispatchIndirect
{
   BufferRef indirect_buffer{};
};

struct CopyTextureToBuffer
{
   TextureRef src_texture;
   BufferRef dst_buffer;
};

struct CopyBufferToTexture
{
   BufferRef src_buffer;
   TextureRef dst_texture;
};

struct CopyBuffer
{
   BufferRef src_buffer;
   BufferRef dst_buffer;
};

struct CopyTexture
{
   TextureRef src_texture;
   TextureRef dst_texture;
};

struct CopyTextureRegion
{
   TextureRef src_texture;
   Vector2i src_offset;
   TextureRef dst_texture;
   Vector2i dst_offset;
   Vector2i size;
};

struct BlitTexture
{
   TextureRef src_texture;
   TextureRef dst_texture;
};

struct FillBuffer
{
   Name buff_name{};
   std::vector<u8> data{};
};

struct BeginRenderPass
{
   Name pass_name;
   std::vector<Name> render_targets;
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
   Name tex_name;
   graphics_api::PipelineStage pipeline_stage;
   graphics_api::TextureState state;
};

struct ExportBuffer
{
   Name buff_name;
   graphics_api::PipelineStage pipeline_stage;
   graphics_api::BufferAccess access;
};

struct PushConstant
{
   graphics_api::PipelineStageFlags stage_flags;
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
   bool should_reset_timestamp_query_pool;
};

struct QueryTimestamp
{
   u32 index;
   bool is_closing;
};

struct BeginQuery
{
   u32 index;
};

struct EndQuery
{
   u32 index;
};

struct SetViewport
{
   Vector4 dimensions;
   float min_depth;
   float max_depth;
};

}// namespace cmd

using Command =
   std::variant<cmd::BindGraphicsPipeline, cmd::BindComputePipeline, cmd::BindRayTracingPipeline, cmd::DrawPrimitives,
                cmd::DrawIndexedPrimitives, cmd::DrawIndexedIndirectWithCount, cmd::DrawIndirectWithCount, cmd::Dispatch,
                cmd::DispatchIndirect, cmd::BindDescriptors, cmd::BindVertexBuffer, cmd::BindIndexBuffer, cmd::CopyTextureToBuffer,
                cmd::CopyBufferToTexture, cmd::CopyBuffer, cmd::CopyTexture, cmd::CopyTextureRegion, cmd::BlitTexture,
                cmd::PlaceTextureBarrier, cmd::PlaceBufferBarrier, cmd::FillBuffer, cmd::BeginRenderPass, cmd::EndRenderPass,
                cmd::IfEnabledCond, cmd::IfDisabledCond, cmd::EndIfCond, cmd::ExportTexture, cmd::ExportBuffer, cmd::PushConstant,
                cmd::TraceRays, cmd::ResetQueries, cmd::QueryTimestamp, cmd::BeginQuery, cmd::EndQuery, cmd::SetViewport>;

}// namespace triglav::render_core::detail
