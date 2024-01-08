#pragma once

#include "Name.hpp"

namespace renderer {

struct MaterialProps
{
   alignas(4) int hasNormalMap{0};
   alignas(4) float diffuseAmount{};
};

struct Material
{
   Name texture{};
   Name normal_texture{};
   MaterialProps props{};
};

}// namespace renderer