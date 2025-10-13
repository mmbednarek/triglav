#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/font/FontManager.hpp"
#include "triglav/font/Typeface.hpp"
#include "triglav/io/Path.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Typeface>
{
   constexpr static ResourceLoadType type{ResourceLoadType::Font};

   static font::Typeface load_font(const font::FontManger& manager, const io::Path& path);
};

}// namespace triglav::resource
