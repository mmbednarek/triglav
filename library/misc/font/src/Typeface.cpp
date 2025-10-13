#include "Typeface.hpp"

#include <cstring>
#include <freetype/freetype.h>
#include <utility>

namespace triglav::font {

Typeface::Typeface(const FT_Face face) :
    m_face(face)
{
}

Typeface::~Typeface()
{
   auto faceAccess = m_face.access();
   FT_Done_Face(*faceAccess);
}

Typeface::Typeface(Typeface&& other) noexcept :
    m_face(std::exchange(other.m_face.access().value(), nullptr))
{
}

Typeface& Typeface::operator=(Typeface&& other) noexcept
{
   auto otherFaceAccess = other.m_face.access();
   auto faceAccess = m_face.access();
   *faceAccess = std::exchange(*otherFaceAccess, nullptr);
   return *this;
}

std::optional<RenderedRune> Typeface::render_glyph(const int size, const Rune rune) const
{
   auto faceAccess = m_face.const_access();

   if (auto err = FT_Set_Pixel_Sizes(*faceAccess, 0, size); err != FT_Err_Ok) {
      return std::nullopt;
   }

   const auto index = FT_Get_Char_Index(*faceAccess, rune);

   if (auto err = FT_Load_Glyph(*faceAccess, index, FT_LOAD_DEFAULT); err != FT_Err_Ok) {
      return std::nullopt;
   }
   if (auto err = FT_Render_Glyph(faceAccess.value()->glyph, FT_RENDER_MODE_NORMAL); err != FT_Err_Ok) {
      return std::nullopt;
   }

   std::vector<u8> data(faceAccess.value()->glyph->bitmap.width * faceAccess.value()->glyph->bitmap.rows);
   std::memcpy(data.data(), faceAccess.value()->glyph->bitmap.buffer, sizeof(u8) * data.size());

   return RenderedRune{
      std::move(data),
      faceAccess.value()->glyph->bitmap.width,
      faceAccess.value()->glyph->bitmap.rows,
      static_cast<i32>(faceAccess.value()->glyph->advance.x >> 6),
      static_cast<i32>(faceAccess.value()->glyph->advance.y >> 6),
      faceAccess.value()->glyph->bitmap_left,
      faceAccess.value()->glyph->bitmap_top,
   };
}

}// namespace triglav::font