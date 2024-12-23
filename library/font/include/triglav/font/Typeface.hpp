#pragma once

#include "FreeTypeForwardDecl.hpp"

#include "triglav/Int.hpp"
#include "triglav/threading/SafeAccess.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace triglav::font {

using Rune = uint32_t;

struct RenderedRune
{
   std::vector<u8> data;
   u32 width;
   u32 height;
   i32 advanceX;
   i32 advanceY;
   i32 bitmapLeft;
   i32 bitmapTop;
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

   [[nodiscard]] std::optional<RenderedRune> render_glyph(int size, Rune rune) const;

 private:
   threading::SafeAccess<FT_Face> m_face{};
};

}// namespace triglav::font