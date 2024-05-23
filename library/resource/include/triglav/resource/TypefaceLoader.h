#pragma once

#include "Loader.hpp"

#include "triglav/font/FontManager.h"
#include "triglav/font/Typeface.h"
#include "triglav/io/Path.h"
#include "triglav/Name.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Typeface>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Font};

   static font::Typeface load_font(const font::FontManger &manager, const io::Path& path);
};

}// namespace triglav::resource
