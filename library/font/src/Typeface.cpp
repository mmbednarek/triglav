#include "Typeface.h"

#include <freetype/freetype.h>
#include <utility>

namespace triglav::font {

Typeface::Typeface(const FT_Face face) :
    m_face(face)
{
}

Typeface::~Typeface()
{
   FT_Done_Face(m_face);
}

Typeface::Typeface(Typeface &&other) noexcept :
    m_face(std::exchange(other.m_face, nullptr))
{
}

Typeface &Typeface::operator=(Typeface &&other) noexcept
{
   m_face = std::exchange(other.m_face, nullptr);
   return *this;
}

RenderedRune Typeface::render_glyph(const int size, const Rune rune) const
{
   FT_Set_Pixel_Sizes(m_face, 0, size);
   const auto index = FT_Get_Char_Index(m_face, rune);
   FT_Load_Glyph(m_face, index, FT_LOAD_DEFAULT);
   FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL);

   return RenderedRune{
           m_face->glyph->bitmap.buffer,
           m_face->glyph->bitmap.width,
           m_face->glyph->bitmap.rows,
           m_face->glyph->advance.x >> 6,
           m_face->glyph->advance.y >> 6,
           m_face->glyph->bitmap_left,
           m_face->glyph->bitmap_top,
   };
}

}// namespace font