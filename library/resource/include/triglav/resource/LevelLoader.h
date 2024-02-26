#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/world/Level.h"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Level>
{
   constexpr static bool is_gpu_resource{false};
   constexpr static bool is_font_resource{false};

   static world::Level load(std::string_view path);
};

}// namespace triglav::resource
