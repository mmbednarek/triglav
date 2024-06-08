#pragma once

#include "triglav/ResourceType.hpp"

namespace triglav::resource {

enum class ResourceLoadType
{
   None,
   Static,
   StaticDependent,
   Font,
   Graphics,
   GraphicsDependent,
};

template<ResourceType CResourceType>
struct Loader
{
   constexpr static ResourceLoadType type{ResourceLoadType::None};
};

}// namespace triglav::resource