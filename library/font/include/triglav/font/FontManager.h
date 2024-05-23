#pragma once

#include "FreeTypeForwardDecl.h"
#include "Typeface.h"

#include "triglav/io/Path.h"

#include <memory>
#include <string_view>

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

   [[nodiscard]] Typeface create_typeface(const io::Path& path, int variant) const;

 private:
   FT_Library m_library{};
};


}// namespace font