#include "Format.hpp"

#include "triglav/meta/Meta.hpp"

namespace triglav::render_core {

namespace {

// clang-format off
std::array<FormatInfo, static_cast<mem_size>(Format::MaxValue)> FORMAT_INFOS{
   /* R_SRGB8 */   FormatInfo{1, 8},
   /* R_UNorm8 */  FormatInfo{1, 8},
   /* R_UNorm16 */ FormatInfo{1, 16},
   /* R_UInt8 */   FormatInfo{1, 8},
   /* R_UInt32 */  FormatInfo{1, 32},
   /* R_SInt32 */  FormatInfo{1, 32},
   /* R_Float16 */ FormatInfo{1, 16},
   /* R_Float32 */ FormatInfo{1, 32},

   /* RG_SRGB8 */   FormatInfo{2, 16},
   /* RG_UNorm8 */  FormatInfo{2, 16},
   /* RG_UNorm16 */ FormatInfo{2, 32},
   /* RG_UInt8 */   FormatInfo{2, 16},
   /* RG_UInt32 */  FormatInfo{2, 64},
   /* RG_SInt32 */  FormatInfo{2, 64},
   /* RG_Float16 */ FormatInfo{2, 32},
   /* RG_Float32 */ FormatInfo{2, 64},

   /* RGB_SRGB */    FormatInfo{3, 24},
   /* RGB_UNorm8 */  FormatInfo{3, 24},
   /* RGB_UNorm16 */ FormatInfo{3, 48},
   /* RGB_UInt8 */   FormatInfo{3, 24},
   /* RGB_UInt32 */  FormatInfo{3, 96},
   /* RGB_SInt32 */  FormatInfo{3, 96},
   /* RGB_Float16 */ FormatInfo{3, 48},
   /* RGB_Float32 */ FormatInfo{3, 96},

   /* RGBA_SRGB8 */   FormatInfo{4, 32},
   /* RGBA_UNorm8 */  FormatInfo{4, 32},
   /* RGBA_UNorm16 */ FormatInfo{4, 64},
   /* RGBA_UInt8 */   FormatInfo{4, 32},
   /* RGBA_UInt32 */  FormatInfo{4, 128},
   /* RGBA_SInt32 */  FormatInfo{4, 128},
   /* RGBA_Float16 */ FormatInfo{4, 64},
   /* RGBA_Float32 */ FormatInfo{4, 128},

   /* BGRA_SRGB8 */ {4, 32},
   /* BGRA_UInt8 */ {4, 32},

   /* D_UNorm16_S_UInt8 */ {2, 24},
   /* D_UNorm16 */ {1, 16},
   /* D_Float32 */ {1, 32},

   /* BC1_RGB_SRGB_Block */ {3, 0},
   /* BC1_RGB_UNorm_Block */ {3, 0},
   /* BC3_SRGB_Block */ {4, 0},
   /* BC4_UNorm_Block */ {1, 0},
};
// clang-format on

}// namespace

const FormatInfo& format_info(const Format format)
{
   return FORMAT_INFOS[static_cast<int>(format)];
}

graphics_api::ColorFormat to_color_format(const Format format)
{
   switch (format) {
   case Format::R_SRGB8:
      return GAPI_FORMAT(R, sRGB);
   case Format::R_UNorm8:
      return GAPI_FORMAT(R, UNorm8);
   case Format::R_UNorm16:
      return GAPI_FORMAT(R, UNorm16);
   case Format::R_UInt8:
      return GAPI_FORMAT(R, UNorm8);
   case Format::R_UInt32:
      return GAPI_FORMAT(R, UInt32);
   case Format::R_SInt32:
      return GAPI_FORMAT(R, SInt32);
   case Format::R_Float16:
      return GAPI_FORMAT(R, Float16);
   case Format::R_Float32:
      return GAPI_FORMAT(R, Float32);
   case Format::RG_SRGB8:
      return GAPI_FORMAT(RG, sRGB);
   case Format::RG_UNorm8:
      return GAPI_FORMAT(RG, UNorm8);
   case Format::RG_UNorm16:
      return GAPI_FORMAT(RG, UNorm16);
   case Format::RG_UInt8:
      return GAPI_FORMAT(RG, UInt8);
   case Format::RG_UInt32:
      return GAPI_FORMAT(RG, UInt32);
   case Format::RG_SInt32:
      return GAPI_FORMAT(RG, SInt32);
   case Format::RG_Float16:
      return GAPI_FORMAT(RG, Float16);
   case Format::RG_Float32:
      return GAPI_FORMAT(RG, Float32);
   case Format::RGB_SRGB:
      return GAPI_FORMAT(RGB, sRGB);
   case Format::RGB_UNorm8:
      return GAPI_FORMAT(RGB, UNorm8);
   case Format::RGB_UNorm16:
      return GAPI_FORMAT(RGB, UNorm16);
   case Format::RGB_UInt8:
      return GAPI_FORMAT(RGB, UInt8);
   case Format::RGB_UInt32:
      return GAPI_FORMAT(RGB, UInt32);
   case Format::RGB_SInt32:
      return GAPI_FORMAT(RGB, SInt32);
   case Format::RGB_Float16:
      return GAPI_FORMAT(RGB, Float16);
   case Format::RGB_Float32:
      return GAPI_FORMAT(RGB, Float32);
   case Format::RGBA_SRGB8:
      return GAPI_FORMAT(RGBA, sRGB);
   case Format::RGBA_UNorm8:
      return GAPI_FORMAT(RGBA, UNorm8);
   case Format::RGBA_UNorm16:
      return GAPI_FORMAT(RGBA, UNorm16);
   case Format::RGBA_UInt8:
      return GAPI_FORMAT(RGBA, UInt8);
   case Format::RGBA_UInt32:
      return GAPI_FORMAT(RGBA, UInt32);
   case Format::RGBA_SInt32:
      return GAPI_FORMAT(RGBA, SInt32);
   case Format::RGBA_Float16:
      return GAPI_FORMAT(RGBA, Float16);
   case Format::RGBA_Float32:
      return GAPI_FORMAT(RGBA, Float32);
   case Format::BGRA_SRGB8:
      return GAPI_FORMAT(BGRA, sRGB);
   case Format::BGRA_UInt8:
      return GAPI_FORMAT(BGRA, UNorm8);
   case Format::D_UNorm16_S_UInt8:
      return GAPI_FORMAT(DS, UNorm16, UInt8);
   case Format::D_UNorm16:
      return GAPI_FORMAT(D, UNorm16);
   case Format::D_Float32:
      return GAPI_FORMAT(D, Float32);
   case Format::BC1_RGB_SRGB_Block:
      return GAPI_FORMAT(RGB, sRGB_BC1);
   case Format::BC1_RGB_UNorm_Block:
      return GAPI_FORMAT(RGB, UNorm8_BC1);
   case Format::BC3_SRGB_Block:
      return GAPI_FORMAT(RGBA, sRGB_BC3);
   case Format::BC4_UNorm_Block:
      return GAPI_FORMAT(RGBA, sRGB_BC3);
   case Format::MaxValue:
      break;
   }

   std::unreachable();
}


}// namespace triglav::render_core

#define TG_TYPE(ns) ns(triglav, ns(render_core, Format))
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(R_SRGB8)
TG_META_ENUM_VALUE(R_UNorm8)
TG_META_ENUM_VALUE(R_UNorm16)
TG_META_ENUM_VALUE(R_UInt8)
TG_META_ENUM_VALUE(R_UInt32)
TG_META_ENUM_VALUE(R_SInt32)
TG_META_ENUM_VALUE(R_Float16)
TG_META_ENUM_VALUE(R_Float32)

TG_META_ENUM_VALUE(RG_SRGB8)
TG_META_ENUM_VALUE(RG_UNorm8)
TG_META_ENUM_VALUE(RG_UNorm16)
TG_META_ENUM_VALUE(RG_UInt8)
TG_META_ENUM_VALUE(RG_UInt32)
TG_META_ENUM_VALUE(RG_SInt32)
TG_META_ENUM_VALUE(RG_Float16)
TG_META_ENUM_VALUE(RG_Float32)

TG_META_ENUM_VALUE(RGB_SRGB)
TG_META_ENUM_VALUE(RGB_UNorm8)
TG_META_ENUM_VALUE(RGB_UNorm16)
TG_META_ENUM_VALUE(RGB_UInt8)
TG_META_ENUM_VALUE(RGB_UInt32)
TG_META_ENUM_VALUE(RGB_SInt32)
TG_META_ENUM_VALUE(RGB_Float16)
TG_META_ENUM_VALUE(RGB_Float32)

TG_META_ENUM_VALUE(RGBA_SRGB8)
TG_META_ENUM_VALUE(RGBA_UNorm8)
TG_META_ENUM_VALUE(RGBA_UNorm16)
TG_META_ENUM_VALUE(RGBA_UInt8)
TG_META_ENUM_VALUE(RGBA_UInt32)
TG_META_ENUM_VALUE(RGBA_SInt32)
TG_META_ENUM_VALUE(RGBA_Float16)
TG_META_ENUM_VALUE(RGBA_Float32)

TG_META_ENUM_VALUE(BGRA_SRGB8)
TG_META_ENUM_VALUE(BGRA_UInt8)

TG_META_ENUM_VALUE(D_UNorm16_S_UInt8)
TG_META_ENUM_VALUE(D_UNorm16)
TG_META_ENUM_VALUE(D_Float32)

TG_META_ENUM_VALUE(BC1_RGB_SRGB_Block)
TG_META_ENUM_VALUE(BC1_RGB_UNorm_Block)
TG_META_ENUM_VALUE(BC3_SRGB_Block)
TG_META_ENUM_VALUE(BC4_UNorm_Block)
TG_META_ENUM_END
#undef TG_TYPE
