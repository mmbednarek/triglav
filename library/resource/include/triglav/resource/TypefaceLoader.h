#pragma once

#include "Loader.hpp"

#include "triglav/font/FontManager.h"
#include "triglav/font/Typeface.h"
#include "triglav/Name.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Typeface>
{
   constexpr static bool is_gpu_resource{false};
   constexpr static bool is_font_resource{true};

   static font::Typeface load_font(const font::FontManger &manager, std::string_view path);
};

}// namespace triglav::resource
