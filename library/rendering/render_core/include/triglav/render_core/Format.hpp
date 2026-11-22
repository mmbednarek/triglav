#pragma once

#include "triglav/graphics_api/GraphicsApi.hpp"

namespace triglav::render_core {

enum class Format
{
   R_SRGB8,
   R_UNorm8,
   R_UNorm16,
   R_UInt8,
   R_UInt32,
   R_SInt32,
   R_Float16,
   R_Float32,

   RG_SRGB8,
   RG_UNorm8,
   RG_UNorm16,
   RG_UInt8,
   RG_UInt32,
   RG_SInt32,
   RG_Float16,
   RG_Float32,

   RGB_SRGB,
   RGB_UNorm8,
   RGB_UNorm16,
   RGB_UInt8,
   RGB_UInt32,
   RGB_SInt32,
   RGB_Float16,
   RGB_Float32,

   RGBA_SRGB8,
   RGBA_UNorm8,
   RGBA_UNorm16,
   RGBA_UInt8,
   RGBA_UInt32,
   RGBA_SInt32,
   RGBA_Float16,
   RGBA_Float32,

   BGRA_SRGB8,
   BGRA_UInt8,

   D_UNorm16_S_UInt8,
   D_UNorm16,
   D_Float32,

   BC1_RGB_SRGB_Block,
   BC1_RGB_UNorm_Block,
   BC3_SRGB_Block,
   BC4_UNorm_Block,

   MaxValue,
};

struct FormatInfo
{
   u32 channel_count{};
   u32 size{};
};

const FormatInfo& format_info(Format format);
graphics_api::ColorFormat to_color_format(Format format);

}// namespace triglav::render_core