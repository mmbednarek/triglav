#pragma once

#include "../RenderCore.hpp"

namespace triglav::render_core::detail {

namespace descriptor {

struct RWTexture
{
   TextureRef texRef;
};

struct SamplableTexture
{
   TextureRef texRef;
};

struct Texture
{
   TextureRef texRef;
};

struct SampledTextureArray
{
   std::vector<TextureRef> texRefs;
};

struct UniformBuffer
{
   BufferRef buffRef{};
};

struct UniformBufferRange
{
   BufferRef buffRef{};
   u32 offset{};
   u32 size{};
};

struct StorageBuffer
{
   BufferRef buffRef{};
};

struct UniformBufferArray
{
   std::vector<BufferRef> buffers{};
};

struct AccelerationStructure
{
   graphics_api::ray_tracing::AccelerationStructure* accelerationStructure;
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
