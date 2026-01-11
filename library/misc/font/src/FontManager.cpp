#include "FontManager.hpp"

#include "Charset.hpp"

#include <freetype/freetype.h>
#include <stdexcept>

namespace triglav::font {

struct FontMangerInternal
{
   FT_Library library;
};

FontManger::FontManger()
{
   const auto err = FT_Init_FreeType(&m_library);
   if (err != 0) {
      throw std::runtime_error("failed to initialize font library");
   }
}

FontManger::~FontManger()
{
   FT_Done_FreeType(m_library);
}

Typeface FontManger::create_typeface(const io::Path& path, const int variant) const
{
   FT_Face face;
   const auto err = FT_New_Face(m_library, path.string().data(), variant, &face);
   if (err != 0) {
      throw std::runtime_error("failed to create typeface");
   }

   return Typeface(face);
}

}// namespace triglav::font