#pragma once

#include "triglav/ResourceType.hpp"

namespace triglav::resource {

template<ResourceType CResourceType>
struct Loader
{
  constexpr static bool is_gpu_resource{false};
};

}// namespace triglav::resource