#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

namespace graphics_api {
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
   RGBA,
   BGRA,
   DS,
   D
};

enum class ColorFormatPart
{
   Unknown,
   sRGB,
   UNorm16,
   UInt,
   Float32
};

enum class ColorSpace
{
   sRGB,
   HDR
};

enum class ShaderStage : uint32_t
{
   None     = 0,
   Vertex   = (1 << 0),
   Fragment = (1 << 1),
};

using ShaderStageFlags = std::underlying_type_t<ShaderStage>;

constexpr ShaderStageFlags operator|(ShaderStage lhs, ShaderStage rhs)
{
   return static_cast<ShaderStageFlags>(lhs) | static_cast<ShaderStageFlags>(rhs);
}

constexpr ShaderStageFlags operator|(ShaderStageFlags lhs, ShaderStage rhs)
{
   return lhs | static_cast<ShaderStageFlags>(rhs);
}

constexpr bool operator&(ShaderStageFlags lhs, ShaderStage rhs)
{
   return (lhs & static_cast<ShaderStageFlags>(rhs)) != 0;
}

struct ColorFormat
{
   ColorFormatOrder order;
   std::array<ColorFormatPart, 4> parts;
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

namespace ColorPalette {
constexpr Color Black{0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color Red{1.0f, 0.0f, 0.0f, 1.0f};
}// namespace ColorPalette

struct VertexInputAttribute
{
   uint32_t location{};
   ColorFormat format{};
   size_t offset{};
};

struct VertexInputLayout
{
   std::span<VertexInputAttribute> attributes{};
   size_t structure_size{};
};

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
   MultisampleImage,
};

struct DescriptorBinding
{
   int binding{};
   int descriptorCount{};
   DescriptorType type{};
   ShaderStageFlags shaderStages{};
};

enum class AttachmentType
{
   ColorAttachment,
   DepthAttachment,
   ResolveAttachment,
};

enum class SampleCount : uint32_t
{
   Bits1  = (1 << 0),
   Bits2  = (1 << 1),
   Bits4  = (1 << 2),
   Bits8  = (1 << 3),
   Bits16 = (1 << 4),
   Bits32 = (1 << 5),
   Bits64 = (1 << 6),
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

}// namespace graphics_api

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

#define GAPI_PREPEND_FORMAT_PART(value) ::graphics_api::ColorFormatPart::value,

#define GAPI_COLOR_FORMAT(fmt_order, ...)                          \
   ::graphics_api::ColorFormat                                     \
   {                                                               \
      .order = ::graphics_api::ColorFormatOrder::fmt_order, .parts \
      {                                                            \
         GAPI_FOR_EACH(GAPI_PREPEND_FORMAT_PART, __VA_ARGS__)      \
      }                                                            \
   }

#define GAPI_CHECK(stmt) ::graphics_api::check_result(stmt, #stmt)
#define GAPI_CHECK_STATUS(stmt) ::graphics_api::check_status(stmt, #stmt)

#ifdef _WIN32
#define GAPI_PLATFORM_WINDOWS 1
#elif __linux__
#define GAPI_PLATFORM_WAYLAND 1
#endif
