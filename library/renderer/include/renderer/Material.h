#pragma once

#include "Name.hpp"

namespace renderer {

struct MaterialProps
{
   alignas(4) int hasNormalMap{0};
   alignas(4) float roughness{1.0f};
   alignas(4) float metalness{0.0f};
};

struct Material
{
   Name texture{};
   Name normal_texture{};
   MaterialProps props{};
};

}// namespace renderer