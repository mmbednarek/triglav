#pragma once

#include <memory>
#include <string_view>

#include "FreeTypeForwardDecl.h"
#include "Typeface.h"

namespace triglav::font {

struct FontMangerInternal;

class FontManger
{
 public:
   FontManger();
   ~FontManger();

   FontManger(const FontManger &other)                = delete;
   FontManger &operator=(const FontManger &other)     = delete;
   FontManger(FontManger &&other) noexcept            = delete;
   FontManger &operator=(FontManger &&other) noexcept = delete;

   [[nodiscard]] Typeface create_typeface(std::string_view path, int variant) const;

 private:
   FT_Library m_library{};
};


}// namespace font