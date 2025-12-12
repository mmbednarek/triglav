#pragma once

#include "../RenderCore.hpp"

namespace triglav::render_core::detail {

namespace descriptor {

struct RWTexture
{
   TextureRef tex_ref;
};

struct SamplableTexture
{
   TextureRef tex_ref;
};

struct Texture
{
   TextureRef tex_ref;
};

struct SampledTextureArray
{
   std::vector<TextureRef> tex_refs;
};

struct UniformBuffer
{
   BufferRef buff_ref{};
};

struct UniformBufferRange
{
   BufferRef buff_ref{};
   u32 offset{};
   u32 size{};
};

struct StorageBuffer
{
   BufferRef buff_ref{};
};

struct UniformBufferArray
{
   std::vector<BufferRef> buffers{};
};

struct AccelerationStructure
{
   graphics_api::ray_tracing::AccelerationStructure* acceleration_structure;
};

}// namespace descriptor

using Descriptor = std::variant<descriptor::RWTexture, descriptor::SamplableTexture, descriptor::SampledTextureArray, descriptor::Texture,
                                descriptor::UniformBuffer, descriptor::UniformBufferRange, descriptor::UniformBufferArray,
                                descriptor::StorageBuffer, descriptor::AccelerationStructure>;

struct DescriptorAndStage
{
   Descriptor descriptor;
   graphics_api::PipelineStageFlags stages;
};

}// namespace triglav::render_core::detail
