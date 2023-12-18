#pragma once

#include <array>
#include <tuple>

#include "renderer/Name.hpp"

namespace renderer {

using NamePath = std::tuple<Name, std::string_view>;

constexpr std::array g_assetMap{
  // ----- Textures -----
        NamePath{  "tex:earth"_name,           "texture/earth.png"},
        NamePath{  "tex:house"_name,           "texture/house.png"},
        NamePath{ "tex:skybox"_name,          "texture/skybox.png"},
        NamePath{   "tex:bark"_name,            "texture/bark.png"},
        NamePath{ "tex:leaves"_name,          "texture/leaves.png"},
        NamePath{   "tex:gold"_name,            "texture/gold.png"},
        NamePath{  "tex:grass"_name,           "texture/grass.png"},
 // ----- Models -----
        NamePath{  "mdl:house"_name,           "model/house.model"},
        NamePath{   "mdl:tree"_name,            "model/tree.model"},
        NamePath{   "mdl:cage"_name,            "model/cage.model"},
        NamePath{"mdl:terrain"_name,         "model/terrain.model"},
 // ----- Shaders -----
        NamePath{"vsh:example"_name,   "shader/example_vertex.spv"},
        NamePath{"fsh:example"_name, "shader/example_fragment.spv"},
        NamePath{ "fsh:skybox"_name,  "shader/skybox/fragment.spv"},
        NamePath{ "vsh:skybox"_name,    "shader/skybox/vertex.spv"},
};

}// namespace renderer