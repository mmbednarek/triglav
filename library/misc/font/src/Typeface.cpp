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
   auto face_access = m_face.access();
   FT_Done_Face(*face_access);
}

Typeface::Typeface(Typeface&& other) noexcept :
    m_face(std::exchange(other.m_face.access().value(), nullptr))
{
}

Typeface& Typeface::operator=(Typeface&& other) noexcept
{
   auto other_face_access = other.m_face.access();
   auto face_access = m_face.access();
   *face_access = std::exchange(*other_face_access, nullptr);
   return *this;
}

std::optional<RenderedRune> Typeface::render_glyph(const int size, const Rune rune) const
{
   auto face_access = m_face.const_access();

   if (auto err = FT_Set_Pixel_Sizes(*face_access, 0, size); err != FT_Err_Ok) {
      return std::nullopt;
   }

   const auto index = FT_Get_Char_Index(*face_access, rune);

   if (auto err = FT_Load_Glyph(*face_access, index, FT_LOAD_DEFAULT); err != FT_Err_Ok) {
      return std::nullopt;
   }
   if (auto err = FT_Render_Glyph(face_access.value()->glyph, FT_RENDER_MODE_NORMAL); err != FT_Err_Ok) {
      return std::nullopt;
   }

   std::vector<u8> data(face_access.value()->glyph->bitmap.width * face_access.value()->glyph->bitmap.rows);
   std::memcpy(data.data(), face_access.value()->glyph->bitmap.buffer, sizeof(u8) * data.size());

   return RenderedRune{
      std::move(data),
      face_access.value()->glyph->bitmap.width,
      face_access.value()->glyph->bitmap.rows,
      static_cast<i32>(face_access.value()->glyph->advance.x >> 6),
      static_cast<i32>(face_access.value()->glyph->advance.y >> 6),
      face_access.value()->glyph->bitmap_left,
      face_access.value()->glyph->bitmap_top,
   };
}

}// namespace triglav::font