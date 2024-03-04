#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <variant>

#include <glm/vec2.hpp>

namespace triglav::graphics_api {

enum class Status
{
   Success,
   UnsupportedDevice,
   UnsupportedFormat,
   UnsupportedColorSpace,
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
   case ColorFormatOrder::RGB: return 3;
   case ColorFormatOrder::RG: return 2;
   case ColorFormatOrder::R: return 1;
   case ColorFormatOrder::RGBA: return 4;
   case ColorFormatOrder::BGRA: return 4;
   case ColorFormatOrder::DS: return 2;
   case ColorFormatOrder::D: return 1;
   }
   return 0;
}

enum class ColorFormatPart
{
   Unknown,
   sRGB,
   UNorm16,
   UInt,
   Float32,
   Float16
};

constexpr size_t color_format_part_size(const ColorFormatPart part)
{
   switch (part) {
   case ColorFormatPart::Unknown: return 0;
   case ColorFormatPart::sRGB: return 1;
   case ColorFormatPart::UNorm16: return 2;
   case ColorFormatPart::UInt: return 1;
   case ColorFormatPart::Float16: return 2;
   case ColorFormatPart::Float32: return 4;
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
   None           = 0,
   Entrypoint  = (1 << 0),
   VertexShader   = (1 << 1),
   FragmentShader = (1 << 2),
   Transfer       = (1 << 3),
};

struct PipelineStageFlags
{
   using UnderlayingType = std::underlying_type_t<PipelineStage>;

   UnderlayingType value;

   constexpr PipelineStageFlags(const UnderlayingType value) :
       value(value)
   {
   }

   constexpr PipelineStageFlags(const PipelineStage value) :
       value(static_cast<UnderlayingType>(value))
   {
   }

   constexpr bool operator&(PipelineStage rhs) const
   {
      return (this->value & static_cast<UnderlayingType>(rhs)) != 0;
   }

   constexpr PipelineStageFlags operator|(PipelineStage rhs) const
   {
      return PipelineStageFlags{this->value | static_cast<UnderlayingType>(rhs)};
   }

   constexpr PipelineStageFlags operator|(const PipelineStageFlags rhs) const
   {
      return PipelineStageFlags{this->value | rhs.value};
   }
};

constexpr PipelineStageFlags operator|(const PipelineStage lhs, const PipelineStage rhs)
{
   return PipelineStageFlags{std::to_underlying(lhs) | std::to_underlying(rhs)};
}

struct ColorFormat
{
   ColorFormatOrder order;
   std::array<ColorFormatPart, 4> parts;

   [[nodiscard]] size_t pixel_size() const
   {
      size_t result{};
      ColorFormatPart lastPart{};
      const auto count = color_format_order_count(this->order);
      for (size_t i = 0; i < count; ++i) {
         if (parts[i] == ColorFormatPart::Unknown) {
            result += color_format_part_size(lastPart);
         } else {
            result += color_format_part_size(parts[i]);
            lastPart = parts[i];
         }
      }
      return result;
   }
};

struct Resolution
{
   uint32_t width{};
   uint32_t height{};
};

struct Color
{
   float r{}, g{}, b{}, a{};
};

struct DepthStenctilValue
{
   float depthValue{};
   float stencilValue{};
};

struct ClearValue
{
   std::variant<Color, DepthStenctilValue> value;
};

namespace ColorPalette {
constexpr Color Black{0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color Red{1.0f, 0.0f, 0.0f, 1.0f};
}// namespace ColorPalette

enum class DescriptorType
{
   UniformBuffer,
   Sampler,
   ImageSampler,
};

enum class TextureType
{
   SampledImage,
   DepthBuffer,
   SampledDepthBuffer,
   MultisampleImage,
   ColorAttachment,
};

enum class TextureState
{
   Undefined,
   TransferSource,
   TransferDestination,
   ShaderRead,
};

class Texture;

struct TextureBarrierInfo
{
   const Texture *texture{};
   TextureState sourceState;
   TextureState targetState;
   int baseMipLevel{};
   int mipLevelCount{};
};

struct TextureRegion
{
   glm::vec2 offsetMin{};
   glm::vec2 offsetMax{};
   int mipLevel{};
};

enum class SampleCount : uint32_t
{
   Single     = (1 << 0),
   Double     = (1 << 1),
   Quadruple  = (1 << 2),
   Octuple    = (1 << 3),
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
   Clockwise,
   CounterClockwise,
};

enum class AttachmentType
{
   Color,
   Depth,
   Presentable,
};

enum class AttachmentLifetime
{
   // Clear before rendering, preserve after rendering
   ClearPreserve,
   // Clear before rendering, discard output
   ClearDiscard,
   // Keep data before rendering, preserve after redering
   KeepPreserve,
   // Keep data before rendering, discard output
   KeepDiscard,
   // Ignore image data, preserve output
   IgnorePreserve,
   // Ignore image data, discard output
   IgnoreDiscard,
};

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

struct SamplerInfo
{
   FilterType minFilter{};
   FilterType magFilter{};
   TextureAddressMode addressU{};
   TextureAddressMode addressV{};
   TextureAddressMode addressW{};
   bool enableAnisotropy{};
   float mipBias{};
   float minLod{};
   float maxLod{};
};

template<typename T>
using Result = std::expected<T, Status>;

class Exception final : public std::exception
{
 public:
   Status status;
   std::string invoked_function;

   Exception(const Status status, const std::string_view invoked_function) :
       status(status),
       invoked_function(std::string(invoked_function))
   {
   }

   [[nodiscard]] const char *what() const noexcept override
   {
      return "graphics_api exception";
   }
};

template<typename TResultValue>
TResultValue check_result(Result<TResultValue> result, const std::string_view message)
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

#define GAPI_EXPAND(...)  GAPI_EXPAND4(GAPI_EXPAND4(GAPI_EXPAND4(GAPI_EXPAND4(__VA_ARGS__))))
#define GAPI_EXPAND4(...) GAPI_EXPAND3(GAPI_EXPAND3(GAPI_EXPAND3(GAPI_EXPAND3(__VA_ARGS__))))
#define GAPI_EXPAND3(...) GAPI_EXPAND2(GAPI_EXPAND2(GAPI_EXPAND2(GAPI_EXPAND2(__VA_ARGS__))))
#define GAPI_EXPAND2(...) GAPI_EXPAND1(GAPI_EXPAND1(GAPI_EXPAND1(GAPI_EXPAND1(__VA_ARGS__))))
#define GAPI_EXPAND1(...) __VA_ARGS__

#define GAPI_FOR_EACH(macro, ...) __VA_OPT__(GAPI_EXPAND(GAPI_FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define GAPI_FOR_EACH_HELPER(macro, a1, ...) \
   macro(a1) __VA_OPT__(GAPI_FOR_EACH_AGAIN GAPI_PARENS(macro, __VA_ARGS__))
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

#define GAPI_CHECK(stmt)        ::triglav::graphics_api::check_result(stmt, #stmt)
#define GAPI_CHECK_STATUS(stmt) ::triglav::graphics_api::check_status(stmt, #stmt)

#ifdef _WIN32
#define GAPI_PLATFORM_WINDOWS 1
#elif __linux__
#define GAPI_PLATFORM_WAYLAND 1
#endif