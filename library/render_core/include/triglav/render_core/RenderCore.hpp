#pragma once

#include <stdexcept>

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Array.hpp"

namespace triglav::render_core {

template<typename TObject>
TObject checkResult(std::expected<TObject, graphics_api::Status>&& object)
{
   if (not object.has_value()) {
      throw std::runtime_error("failed to init graphics_api object");
   }
   return std::move(object.value());
}

inline void checkStatus(const graphics_api::Status status)
{
   if (status != graphics_api::Status::Success) {
      throw std::runtime_error("failed to init graphics_api object");
   }
}

using BindingIndex = u32;
using PipelineHash = u64;

constexpr auto MAX_DESCRIPTOR_COUNT = 16;
constexpr auto FRAMES_IN_FLIGHT_COUNT = 3;

struct DescriptorInfo
{
   graphics_api::PipelineStageFlags pipelineStages;
   graphics_api::DescriptorType descriptorType{graphics_api::DescriptorType::UniformBuffer};
   u32 descriptorCount{1};

   [[nodiscard]] PipelineHash hash() const;
};

struct DescriptorState
{
   std::array<DescriptorInfo, MAX_DESCRIPTOR_COUNT> descriptors;
   u32 descriptorCount{};

   [[nodiscard]] PipelineHash hash() const;
};

struct VertexAttribute
{
   Name name;
   graphics_api::ColorFormat format;
   u32 offset;

   [[nodiscard]] PipelineHash hash() const;
};

struct VertexLayout
{
   u32 stride{};
   std::vector<VertexAttribute> attributes;

   VertexLayout() = default;
   explicit VertexLayout(u32 stride);

   VertexLayout& add(Name name, const graphics_api::ColorFormat& format, u32 offset);

   [[nodiscard]] PipelineHash hash() const;
};

struct PushConstantDesc
{
   graphics_api::PipelineStageFlags flags;
   u32 size;

   [[nodiscard]] PipelineHash hash() const;
};

struct GraphicPipelineState
{
   std::optional<VertexShaderName> vertexShader;
   std::optional<FragmentShaderName> fragmentShader;
   VertexLayout vertexLayout;
   DescriptorState descriptorState;
   std::vector<graphics_api::ColorFormat> renderTargetFormats;
   std::optional<graphics_api::ColorFormat> depthTargetFormat;
   graphics_api::VertexTopology vertexTopology{graphics_api::VertexTopology::TriangleList};
   graphics_api::DepthTestMode depthTestMode{graphics_api::DepthTestMode::Enabled};
   std::vector<PushConstantDesc> pushConstants;
   bool isBlendingEnabled{true};

   [[nodiscard]] PipelineHash hash() const;
};

struct ComputePipelineState
{
   std::optional<ComputeShaderName> computeShader;
   DescriptorState descriptorState;

   [[nodiscard]] PipelineHash hash() const;
};

struct FromLastFrame
{
   Name name;
};

struct External
{
   Name name;
};

struct TextureMip
{
   Name name;
   u32 mipLevel;
};

using TextureRef = std::variant<TextureName, Name, FromLastFrame, External, TextureMip, const graphics_api::Texture*>;
using BufferRef = std::variant<const graphics_api::Buffer*, Name, FromLastFrame, External>;

struct ExecutionBarrier
{
   graphics_api::PipelineStageFlags srcStageFlags{};
   graphics_api::PipelineStageFlags dstStageFlags{};
};

struct BufferBarrier
{
   BufferRef bufferRef;
   graphics_api::PipelineStageFlags srcStageFlags{};
   graphics_api::PipelineStageFlags dstStageFlags{};
   graphics_api::BufferAccessFlags srcBufferAccess{};
   graphics_api::BufferAccessFlags dstBufferAccess{};
};

struct TextureBarrier
{
   TextureRef textureRef;
   graphics_api::PipelineStageFlags srcStageFlags{};
   graphics_api::PipelineStageFlags dstStageFlags{};
   graphics_api::TextureState srcState{};
   graphics_api::TextureState dstState{};
   u32 baseMipLevel{0};
   u32 mipLevelCount{1};
};

static constexpr u32 calculate_mip_count(const Vector2i& dims)
{
   return static_cast<int>(std::floor(std::log2(std::max(dims.x, dims.y)))) + 1;
}

namespace literals {

constexpr FromLastFrame operator""_last_frame(const char* value, const std::size_t count)
{
   return FromLastFrame{detail::hash_string(std::string_view(value, count))};
}

constexpr External operator""_external(const char* value, const std::size_t count)
{
   return External{detail::hash_string(std::string_view(value, count))};
}

}// namespace literals

}// namespace triglav::render_core
