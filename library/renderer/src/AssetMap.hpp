#pragma once

#include <array>
#include <tuple>

#include "renderer/Name.hpp"

namespace renderer {

using NamePath = std::tuple<Name, std::string_view>;

constexpr std::array g_assetMap{
        NamePath{"tex:earth"_name, "texture/earth.png"},
        NamePath{"tex:house"_name, "texture/house.png"},
        NamePath{"msh:house"_name, "model/house.obj"},
        NamePath{"vsh:example"_name, "shader/example_vertex.spv"},
        NamePath{"fsh:example"_name, "shader/example_fragment.spv"},
        NamePath{"fsh:skybox"_name, "shader/skybox/fragment.spv"},
        NamePath{"vsh:skybox"_name, "shader/skybox/vertex.spv"},
        NamePath{"tex:skybox"_name, "texture/skybox.png"},
};

}// namespace renderer