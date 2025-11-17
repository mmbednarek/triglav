#pragma once

#include "triglav/EnumFlags.hpp"
#include "triglav/Int.hpp"
#include "triglav/Math.hpp"

#include <array>
#include <cstdint>
#include <expected>
#include <glm/vec2.hpp>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace triglav::graphics_api {

enum class Status
{
   Success,
   NoSupportedDevicesFound,
   NoDeviceSupportsRequestedFeatures,
   UnsupportedDevice,
   UnsupportedFormat,
   UnsupportedColorSpace,
   OutOfDateSwapchain,
   PSOCreationFailed,
   InvalidTransferDestination,
   InvalidShaderStage,
};

enum class ColorFormatOrder
{
   RGB,
   RG,
   R,
   RGBA,
   BGRA,
   DS,
   D
};

constexpr size_t color_format_order_count(const ColorFormatOrder order)
{
   switch (order) {
   case ColorFormatOrder::RGB:
      return 3;
   case ColorFormatOrder::RG:
      return 2;
   case ColorFormatOrder::R:
      return 1;
   case ColorFormatOrder::RGBA:
      return 4;
   case ColorFormatOrder::BGRA:
      return 4;
   case ColorFormatOrder::DS:
      return 2;
   case ColorFormatOrder::D:
      return 1;
   }
   return 0;
}

enum class ColorFormatPart
{
   Unknown,
   sRGB,
   UNorm8,
   UNorm16,
   UInt,
   Float32,
   Float16,
   sRGB_BC3,
   sRGB_BC1,
   UNorm8_BC1,
   UNorm8_BC4,
};

constexpr size_t color_format_part_size(const ColorFormatPart part)
{
   switch (part) {
   case ColorFormatPart::Unknown:
      return 0;
   case ColorFormatPart::sRGB:
      return 1;
   case ColorFormatPart::UNorm8:
      return 1;
   case ColorFormatPart::sRGB_BC3:
      return 1;
   case ColorFormatPart::sRGB_BC1:
      return 1;
   case ColorFormatPart::UNorm8_BC1:
      return 1;
   case ColorFormatPart::UNorm8_BC4:
      return 1;
   case ColorFormatPart::UNorm16:
      return 2;
   case ColorFormatPart::UInt:
      return 1;
   case ColorFormatPart::Float16:
      return 2;
   case ColorFormatPart::Float32:
      return 4;
   }
   return 0;
}

enum class ColorSpace
{
   sRGB,
   HDR
};

enum class PipelineStage : uint32_t
{
   None = 0,
   Entrypoint = (1 << 0),
   DrawIndirect = (1 << 1),
   VertexInput = (1 << 2),
   VertexShader = (1 << 3),
   FragmentShader = (1 << 4),
   EarlyZ = (1 << 5),
   LateZ = (1 << 6),
   AttachmentOutput = (1 << 7),
   ComputeShader = (1 << 8),

   RayGenerationShader = (1 << 9),
   AnyHitShader = (1 << 10),
   ClosestHitShader = (1 << 11),
   MissShader = (1 << 12),
   IntersectionShader = (1 << 13),
   CallableShader = (1 << 14),

   Transfer = (1 << 15),
   End = (1 << 16),
};

TRIGLAV_DECL_FLAGS(PipelineStage)

enum class MemoryAccess
{
   None = 0,
   Read = (1 << 0),
   Write = (1 << 1),
};

TRIGLAV_DECL_FLAGS(MemoryAccess)

struct ColorFormat
{
   ColorFormatOrder order;
   std::array<ColorFormatPart, 4> parts;

   [[nodiscard]] size_t pixel_size() const
   {
      size_t result{};
      ColorFormatPart last_part{};
      const auto count = color_format_order_count(this->order);
      for (size_t i = 0; i < count; ++i) {
         if (parts[i] == ColorFormatPart::Unknown) {
            result += color_format_part_size(last_part);
         } else {
            result += color_format_part_size(parts[i]);
            last_part = parts[i];
         }
      }
      return result;
   }

   [[nodiscard]] bool is_depth_format() const
   {
      switch (this->order) {
      case ColorFormatOrder::DS:
         [[fallthrough]];
      case ColorFormatOrder::D:
         return true;
      default:
         return false;
      }
   }

   [[nodiscard]] i32 channel_count() const
   {
      switch (this->order) {
      case ColorFormatOrder::RGB:
         return 3;
      case ColorFormatOrder::RG:
         return 2;
      case ColorFormatOrder::R:
         return 1;
      case ColorFormatOrder::RGBA:
         return 4;
      case ColorFormatOrder::BGRA:
         return 4;
      case ColorFormatOrder::DS:
         return 2;
      case ColorFormatOrder::D:
         return 1;
      default:
         return false;
      }
   }

   bool operator==(const ColorFormat& other) const
   {
      return this->order == other.order && this->parts == other.parts;
   }
};

struct Resolution
{
   u32 width{};
   u32 height{};

   Resolution operator*(const float scale) const
   {
      return {static_cast<u32>(static_cast<float>(width) * scale), static_cast<u32>(static_cast<float>(height) * scale)};
   }
};

struct Color
{
   float r{}, g{}, b{}, a{};
};

struct DepthStenctilValue
{
   float depth_value{};
   u32 stencil_value{};
};

struct ClearValue
{
   std::variant<Color, DepthStenctilValue> value;

   static ClearValue color(const Color& color)
   {
      return ClearValue{.value = color};
   }

   static ClearValue depth_stencil(const float depth, const u32 stencil)
   {
      return ClearValue{.value = DepthStenctilValue{depth, stencil}};
   }
};

namespace ColorPalette {
constexpr Color Empty{0.0f, 0.0f, 0.0f, 0.0f};
constexpr Color Black{0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color Red{1.0f, 0.0f, 0.0f, 1.0f};
}// namespace ColorPalette

enum class DescriptorType
{
   UniformBuffer,
   StorageBuffer,
   Sampler,
   ImageSampler,
   ImageOnly,
   StorageImage,
   AccelerationStructure,
};

struct DescriptorBinding
{
   u32 binding;
   DescriptorType type;
   u32 count;
   PipelineStage stage;
};

enum class TextureUsage
{
   None = 0,
   TransferSrc = (1 << 0),
   TransferDst = (1 << 1),
   Sampled = (1 << 2),
   ColorAttachment = (1 << 3),
   DepthStencilAttachment = (1 << 4),
   Transient = (1 << 5),
   Storage = (1 << 6),
};

TRIGLAV_DECL_FLAGS(TextureUsage)

enum class TextureState
{
   Undefined,
   TransferSrc,
   TransferDst,
   ShaderRead,
   General,
   GeneralRead,
   GeneralWrite,
   RenderTarget,
   ReadOnlyRenderTarget,
   Present,
};

[[nodiscard]] constexpr MemoryAccess to_memory_access(const TextureState state)
{
   switch (state) {
   case TextureState::TransferDst:
      [[fallthrough]];
   case TextureState::General:
      [[fallthrough]];
   case TextureState::GeneralWrite:
      [[fallthrough]];
   case TextureState::RenderTarget:
      return MemoryAccess::Write;
   case TextureState::TransferSrc:
      [[fallthrough]];
   case TextureState::ShaderRead:
      [[fallthrough]];
   case TextureState::ReadOnlyRenderTarget:
      [[fallthrough]];
   case TextureState::GeneralRead:
      [[fallthrough]];
   case TextureState::Present:
      return MemoryAccess::Read;
   default:
      return MemoryAccess::None;
   }
}

class Texture;

struct TextureBarrierInfo
{
   const Texture* texture{};
   TextureState source_state;
   TextureState target_state;
   int base_mip_level{};
   int mip_level_count{};
};

struct TextureRegion
{
   glm::vec2 offset_min{};
   glm::vec2 offset_max{};
   int mip_level{};
};

enum class SampleCount : uint32_t
{
   Single = (1 << 0),
   Double = (1 << 1),
   Quadruple = (1 << 2),
   Octuple = (1 << 3),
   Sexdecuple = (1 << 4),
};

enum class VertexTopology
{
   TriangleList,
   TriangleFan,
   TriangleStrip,
   LineStrip,
   LineList,
};

enum class RasterizationMethod
{
   Point,
   Line,
   Fill,
};

enum class Culling
{
   None,
   Clockwise,
   CounterClockwise,
};

enum class AttachmentAttribute
{
   None = 0,
   Color = (1 << 0),
   Depth = (1 << 1),
   Resolve = (1 << 2),
   Presentable = (1 << 3),
   LoadImage = (1 << 4),
   ClearImage = (1 << 5),
   StoreImage = (1 << 6),
   TransferSrc = (1 << 7),
   TransferDst = (1 << 8),
   Storage = (1 << 9),
};

TRIGLAV_DECL_FLAGS(AttachmentAttribute)

enum class FilterType
{
   Linear,
   NearestNeighbour,
};

enum class TextureAddressMode
{
   Repeat,
   Mirror,
   Clamp,
   Border,
   MirrorOnce,
};

struct SamplerProperties
{
   FilterType min_filter{};
   FilterType mag_filter{};
   TextureAddressMode address_u{};
   TextureAddressMode address_v{};
   TextureAddressMode address_w{};
   bool enable_anisotropy{};
   float mip_bias{};
   float min_lod{};
   float max_lod{};
};

enum class WorkType : u32
{
   None = 0,
   Graphics = (1 << 0),
   Transfer = (1 << 1),
   Compute = (1 << 2),
   Presentation = (1 << 3),
};

TRIGLAV_DECL_FLAGS(WorkType)

enum class BufferUsage : u32
{
   None = 0,
   HostVisible = (1 << 0),
   TransferSrc = (1 << 1),
   TransferDst = (1 << 2),
   UniformBuffer = (1 << 3),
   VertexBuffer = (1 << 4),
   IndexBuffer = (1 << 5),
   StorageBuffer = (1 << 6),
   AccelerationStructure = (1 << 7),
   AccelerationStructureRead = (1 << 8),
   ShaderBindingTable = (1 << 9),
   Indirect = (1 << 10),
};

TRIGLAV_DECL_FLAGS(BufferUsage)

enum class DepthTestMode
{
   Disabled,
   Enabled,
   ReadOnly
};

enum class PipelineType
{
   Graphics,
   Compute,
   RayTracing,
};

enum class PresentMode
{
   Fifo,
   Immediate,
   Mailbox,
};

enum class DevicePickStrategy
{
   PreferDedicated,
   PreferIntegrated,
   Any,
};

enum class DeviceFeature : u32
{
   None = 0,
   RayTracing = (1 << 0),
};

TRIGLAV_DECL_FLAGS(DeviceFeature)

enum class BufferAccess
{
   None = 0,
   TransferRead = (1 << 0),
   TransferWrite = (1 << 1),
   UniformRead = (1 << 2),
   IndexRead = (1 << 3),
   VertexRead = (1 << 4),
   ShaderRead = (1 << 5),
   ShaderWrite = (1 << 6),
   IndirectCmdRead = (1 << 7),
   MemoryRead = (1 << 8),
   MemoryWrite = (1 << 9),
};

[[nodiscard]] constexpr MemoryAccess to_memory_access(const BufferAccess access)
{
   switch (access) {
   case BufferAccess::TransferWrite:
      [[fallthrough]];
   case BufferAccess::MemoryWrite:
      [[fallthrough]];
   case BufferAccess::ShaderWrite:
      return MemoryAccess::Write;
   case BufferAccess::IndirectCmdRead:
      [[fallthrough]];
   case BufferAccess::TransferRead:
      [[fallthrough]];
   case BufferAccess::UniformRead:
      [[fallthrough]];
   case BufferAccess::IndexRead:
      [[fallthrough]];
   case BufferAccess::VertexRead:
      [[fallthrough]];
   case BufferAccess::MemoryRead:
      [[fallthrough]];
   case BufferAccess::ShaderRead:
      return MemoryAccess::Read;
   default:
      return MemoryAccess::None;
   }
}

TRIGLAV_DECL_FLAGS(BufferAccess)

struct RenderAttachment
{
   Texture* texture;
   TextureState state;
   AttachmentAttributeFlags flags;
   ClearValue clear_value;
};

struct RenderingInfo
{
   Vector2i render_area_offset;
   Vector2i render_area_extent;
   u32 layer_count;
   u32 view_mask;

   std::vector<RenderAttachment> color_attachments;
   std::optional<RenderAttachment> depth_attachment;
};

class Buffer;

struct BufferBarrier
{
   const Buffer* buffer;
   BufferAccessFlags src_access;
   BufferAccessFlags dst_access;
};

enum class QueryType
{
   PipelineStats,
   Timestamp,
};

template<typename T>
using Result = std::expected<T, Status>;

class Exception final : public std::exception
{
 public:
   Exception(Status status, std::string_view invoked_function);

   [[nodiscard]] const char* what() const noexcept override;

 private:
   std::string m_message;
};

template<typename TResultValue>
TResultValue check_result(Result<TResultValue>&& result, const std::string_view message)
{
   if (not result.has_value()) {
      throw Exception(result.error(), message);
   }
   return std::move(*result);
}

inline void check_status(const Status status, const std::string_view message)
{
   if (status != Status::Success) {
      throw Exception(status, message);
   }
}

}// namespace triglav::graphics_api

#define GAPI_PARENS ()

#define GAPI_EXPAND(...) GAPI_EXPAND4(GAPI_EXPAND4(GAPI_EXPAND4(GAPI_EXPAND4(__VA_ARGS__))))
#define GAPI_EXPAND4(...) GAPI_EXPAND3(GAPI_EXPAND3(GAPI_EXPAND3(GAPI_EXPAND3(__VA_ARGS__))))
#define GAPI_EXPAND3(...) GAPI_EXPAND2(GAPI_EXPAND2(GAPI_EXPAND2(GAPI_EXPAND2(__VA_ARGS__))))
#define GAPI_EXPAND2(...) GAPI_EXPAND1(GAPI_EXPAND1(GAPI_EXPAND1(GAPI_EXPAND1(__VA_ARGS__))))
#define GAPI_EXPAND1(...) __VA_ARGS__

#define GAPI_FOR_EACH(macro, ...) __VA_OPT__(GAPI_EXPAND(GAPI_FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define GAPI_FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(GAPI_FOR_EACH_AGAIN GAPI_PARENS(macro, __VA_ARGS__))
#define GAPI_FOR_EACH_AGAIN() GAPI_FOR_EACH_HELPER

#define GAPI_PREPEND_FORMAT_PART(value) ::triglav::graphics_api::ColorFormatPart::value,

#define GAPI_FORMAT(fmt_order, ...)                                         \
   ::triglav::graphics_api::ColorFormat                                     \
   {                                                                        \
      .order = ::triglav::graphics_api::ColorFormatOrder::fmt_order, .parts \
      {                                                                     \
         GAPI_FOR_EACH(GAPI_PREPEND_FORMAT_PART, __VA_ARGS__)               \
      }                                                                     \
   }

#define GAPI_CHECK(stmt) ::triglav::graphics_api::check_result(stmt, #stmt)
#define GAPI_CHECK_STATUS(stmt) ::triglav::graphics_api::check_status(stmt, #stmt)

#if defined(NDEBUG) || defined(TG_DISABLE_DEBUG_UTILS)
#define TG_SET_DEBUG_NAME(object, name)
#else
#define TG_SET_DEBUG_NAME(object, name) object.set_debug_name(name)
#endif

#ifdef _WIN32
#define GAPI_PLATFORM_WINDOWS 1
#elif __linux__
#define GAPI_PLATFORM_WAYLAND 1
#endif
