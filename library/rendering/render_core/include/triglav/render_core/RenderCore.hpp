#pragma once

#include <stdexcept>

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Array.hpp"

namespace triglav::render_core {

template<typename TObject>
TObject check_result(std::expected<TObject, graphics_api::Status>&& object)
{
   if (not object.has_value()) {
      throw std::runtime_error("failed to init graphics_api object");
   }
   return std::move(object.value());
}

inline void check_status(const graphics_api::Status status)
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
   graphics_api::PipelineStageFlags pipeline_stages;
   graphics_api::DescriptorType descriptor_type{graphics_api::DescriptorType::UniformBuffer};
   u32 descriptor_count{1};

   [[nodiscard]] PipelineHash hash() const;
};

struct DescriptorState
{
   std::array<DescriptorInfo, MAX_DESCRIPTOR_COUNT> descriptors;
   u32 descriptor_count{};

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
   std::optional<VertexShaderName> vertex_shader;
   std::optional<FragmentShaderName> fragment_shader;
   VertexLayout vertex_layout;
   DescriptorState descriptor_state;
   std::vector<graphics_api::ColorFormat> render_target_formats;
   std::optional<graphics_api::ColorFormat> depth_target_format;
   graphics_api::VertexTopology vertex_topology{graphics_api::VertexTopology::TriangleList};
   graphics_api::DepthTestMode depth_test_mode{graphics_api::DepthTestMode::Enabled};
   std::vector<PushConstantDesc> push_constants;
   float line_width{1.0f};
   bool is_blending_enabled{true};

   [[nodiscard]] PipelineHash hash() const;
};

struct ComputePipelineState
{
   std::optional<ComputeShaderName> compute_shader;
   DescriptorState descriptor_state;

   [[nodiscard]] PipelineHash hash() const;
};

enum class RayTracingShaderGroupType
{
   General,
   Triangles,
   Procedural,
};

struct RayTracingShaderGroup
{
   RayTracingShaderGroupType type;
   std::optional<ResourceName> general_shader;
   std::optional<RayClosestHitShaderName> closest_hit_shader;

   [[nodiscard]] PipelineHash hash() const;
};

struct RayTracingPipelineState
{
   std::optional<RayGenShaderName> ray_gen_shader;
   std::vector<RayClosestHitShaderName> ray_closest_hit_shaders;
   std::vector<RayMissShaderName> ray_miss_shaders;
   DescriptorState descriptor_state;
   std::vector<PushConstantDesc> push_constants;
   u32 max_recursion{1};
   std::vector<RayTracingShaderGroup> shader_groups;

   void reset();
   [[nodiscard]] std::vector<Name> shader_bindings() const;
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
   u32 mip_level;
};

using TextureRef = std::variant<TextureName, Name, FromLastFrame, External, TextureMip, const graphics_api::Texture*>;
using BufferRef = std::variant<const graphics_api::Buffer*, Name, FromLastFrame, External>;

struct ExecutionBarrier
{
   graphics_api::PipelineStageFlags src_stage_flags{};
   graphics_api::PipelineStageFlags dst_stage_flags{};
};

struct BufferBarrier
{
   BufferRef buffer_ref;
   graphics_api::PipelineStageFlags src_stage_flags{};
   graphics_api::PipelineStageFlags dst_stage_flags{};
   graphics_api::BufferAccessFlags src_buffer_access{};
   graphics_api::BufferAccessFlags dst_buffer_access{};
};

struct TextureBarrier
{
   TextureRef texture_ref;
   graphics_api::PipelineStageFlags src_stage_flags{};
   graphics_api::PipelineStageFlags dst_stage_flags{};
   graphics_api::TextureState src_state{};
   graphics_api::TextureState dst_state{};
   u32 base_mip_level{0};
   u32 mip_level_count{1};
};

inline u32 calculate_mip_count(const Vector2i& dims)
{
   return static_cast<int>(std::floor(std::log2(std::max(dims.x, dims.y)))) + 1;
}

inline Name make_rt_shader_name(const ResourceName res_name)
{
   return res_name.name() * static_cast<u64>(res_name.type());
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
