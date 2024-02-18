#pragma once

#include "Loader.hpp"

#include "triglav/Name.hpp"
#include "triglav/render_core/Material.hpp"

#include <string_view>

namespace triglav::resource {

template<>
struct Loader<ResourceType::Material>
{
   constexpr static bool is_gpu_resource{false};
   constexpr static bool is_font_resource{false};

   static render_core::Material load(std::string_view path);
};

}// namespace triglav::resource
