#pragma once

#include <array>
#include <tuple>

#include "renderer/Name.hpp"

namespace renderer {

using NamePath = std::tuple<Name, std::string_view>;

constexpr std::array g_assetMap{
  // ----- Textures -----
        NamePath{         "tex:earth"_name,              "texture/earth.png"},
        NamePath{        "tex:skybox"_name,             "texture/skybox.png"},
        NamePath{          "tex:bark"_name,               "texture/bark.png"},
        NamePath{   "tex:bark/normal"_name,        "texture/bark_normal.png"},
        NamePath{        "tex:leaves"_name,             "texture/leaves.png"},
        NamePath{          "tex:gold"_name,               "texture/gold.png"},
        NamePath{         "tex:grass"_name,              "texture/grass.png"},
        NamePath{  "tex:grass/normal"_name,       "texture/grass_normal.png"},
        NamePath{          "tex:pine"_name,               "texture/pine.png"},
        NamePath{   "tex:pine/normal"_name,        "texture/pine_normal.png"},
 // ----- Models -----
        NamePath{          "mdl:tree"_name,               "model/tree.model"},
        NamePath{          "mdl:cage"_name,               "model/cage.model"},
        NamePath{       "mdl:terrain"_name,            "model/terrain.model"},
        NamePath{        "mdl:teapot"_name,             "model/teapot.model"},
        NamePath{         "mdl:bunny"_name,              "model/bunny.model"},
        NamePath{          "mdl:pine"_name,               "model/pine.model"},
 // ----- Shaders -----
        NamePath{         "fsh:model"_name,      "shader/model/fragment.spv"},
        NamePath{         "vsh:model"_name,        "shader/model/vertex.spv"},
        NamePath{        "fsh:skybox"_name,     "shader/skybox/fragment.spv"},
        NamePath{        "vsh:skybox"_name,       "shader/skybox/vertex.spv"},
        NamePath{    "fsh:shadow_map"_name, "shader/shadow_map/fragment.spv"},
        NamePath{    "vsh:shadow_map"_name,   "shader/shadow_map/vertex.spv"},
        NamePath{        "fsh:sprite"_name,     "shader/sprite/fragment.spv"},
        NamePath{        "vsh:sprite"_name,       "shader/sprite/vertex.spv"},
        NamePath{          "fsh:text"_name,       "shader/text/fragment.spv"},
        NamePath{          "vsh:text"_name,         "shader/text/vertex.spv"},
 // ----- Typefaces -----
        NamePath{     "tfc:cantarell"_name,            "fonts/cantarell.ttf"},
        NamePath{"tfc:cantarell/bold"_name,       "fonts/cantarell_bold.ttf"},
};

}// namespace renderer
