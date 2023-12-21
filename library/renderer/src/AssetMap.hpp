#pragma once

#include <array>
#include <tuple>

#include "renderer/Name.hpp"

namespace renderer {

using NamePath = std::tuple<Name, std::string_view>;

constexpr std::array g_assetMap{
  // ----- Textures -----
        NamePath{       "tex:earth"_name,           "texture/earth.png"},
        NamePath{      "tex:skybox"_name,          "texture/skybox.png"},
        NamePath{        "tex:bark"_name,            "texture/bark.png"},
        NamePath{ "tex:bark/normal"_name,     "texture/bark_normal.png"},
        NamePath{      "tex:leaves"_name,          "texture/leaves.png"},
        NamePath{        "tex:gold"_name,            "texture/gold.png"},
        NamePath{       "tex:grass"_name,           "texture/grass.png"},
        NamePath{"tex:grass/normal"_name,    "texture/grass_normal.png"},
        NamePath{        "tex:pine"_name,            "texture/pine.png"},
        NamePath{ "tex:pine/normal"_name,     "texture/pine_normal.png"},
 // ----- Models -----
        NamePath{        "mdl:tree"_name,            "model/tree.model"},
        NamePath{        "mdl:cage"_name,            "model/cage.model"},
        NamePath{     "mdl:terrain"_name,         "model/terrain.model"},
        NamePath{      "mdl:teapot"_name,          "model/teapot.model"},
        NamePath{       "mdl:bunny"_name,           "model/bunny.model"},
        NamePath{        "mdl:pine"_name,            "model/pine.model"},
 // ----- Shaders -----
        NamePath{     "vsh:example"_name,   "shader/example_vertex.spv"},
        NamePath{     "fsh:example"_name, "shader/example_fragment.spv"},
        NamePath{      "fsh:skybox"_name,  "shader/skybox/fragment.spv"},
        NamePath{      "vsh:skybox"_name,    "shader/skybox/vertex.spv"},
};

}// namespace renderer
