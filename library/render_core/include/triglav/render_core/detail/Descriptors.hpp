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

struct StorageBuffer
{
   BufferRef buffRef{};
};

struct UniformBufferArray
{
   std::vector<BufferRef> buffers{};
};

}// namespace descriptor

using Descriptor = std::variant<descriptor::RWTexture, descriptor::SamplableTexture, descriptor::SampledTextureArray, descriptor::Texture,
                                descriptor::UniformBuffer, descriptor::UniformBufferArray, descriptor::StorageBuffer>;

struct DescriptorAndStage
{
   Descriptor descriptor;
   graphics_api::PipelineStage stage;
};

}// namespace triglav::render_core::detail
