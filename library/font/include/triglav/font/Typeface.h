#pragma once

#include "FreeTypeForwardDecl.h"

#include <cstdint>

namespace triglav::font {

using Rune = uint32_t;

struct RenderedRune
{
   uint8_t* data;
   uint32_t width;
   uint32_t height;
   int advanceX;
   int advanceY;
   int bitmapLeft;
   int bitmapTop;
};

class Typeface
{
 public:
   explicit Typeface(FT_Face face);
   ~Typeface();

   Typeface(const Typeface& other) = delete;
   Typeface& operator=(const Typeface& other) = delete;
   Typeface(Typeface&& other) noexcept;
   Typeface& operator=(Typeface&& other) noexcept;

   [[nodiscard]] RenderedRune render_glyph(int size, Rune rune) const;

 private:
   FT_Face m_face{};
};

}// namespace triglav::font