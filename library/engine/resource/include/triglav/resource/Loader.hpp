#pragma once

#include "triglav/Name.hpp"
#include "triglav/ResourceType.hpp"
#include "triglav/io/Path.hpp"

#include <vector>

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

template<typename TLoader>
concept CollectsDependencies = requires(std::vector<ResourceName>& out_deps, const io::Path& path) {
   { TLoader::collect_dependencies(out_deps, path) } -> std::same_as<void>;
};

template<ResourceType CResourceType>
struct Loader
{
   constexpr static ResourceLoadType type{ResourceLoadType::None};
};

}// namespace triglav::resource