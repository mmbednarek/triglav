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

   void add(Name name, const graphics_api::ColorFormat& format, u32 offset);
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

   [[nodiscard]] PipelineHash hash() const;
};

struct ComputePipelineState
{
   std::optional<ComputeShaderName> computeShader;
   DescriptorState descriptorState;

   [[nodiscard]] PipelineHash hash() const;
};

}// namespace triglav::render_core
