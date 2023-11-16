#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <span>

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
   None = 0,
   Vertex = (1 << 0),
   Fragment = (1 << 1),
};

using ShaderStageFlags = std::underlying_type_t<ShaderStage>;

constexpr ShaderStageFlags operator|(ShaderStage lhs, ShaderStage rhs) {
   return static_cast<ShaderStageFlags>(lhs) | static_cast<ShaderStageFlags>(rhs);
}

constexpr ShaderStageFlags operator|(ShaderStageFlags lhs, ShaderStage rhs) {
   return lhs | static_cast<ShaderStageFlags>(rhs);
}

constexpr bool operator&(ShaderStageFlags lhs, ShaderStage rhs) {
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

struct VertexInputLayout {
   std::span<VertexInputAttribute> attributes{};
   size_t structure_size{};
};

enum class DescriptorType {
   UniformBuffer,
   Sampler,
   ImageSampler,
};

enum class TextureType {
   SampledImage,
   DepthBuffer,
   MultisampleImage,
};

struct DescriptorBinding {
   int binding{};
   int descriptorCount{};
   DescriptorType type{};
   ShaderStageFlags shaderStages{};
};

template<typename T>
using Result = std::expected<T, Status>;
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

#ifdef _WIN32
#define GAPI_PLATFORM_WINDOWS 1
#elif __linux__
#define GAPI_PLATFORM_WAYLAND 1
#endif
