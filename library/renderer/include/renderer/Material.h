#pragma once

#include "Name.hpp"

namespace renderer {

struct Material
{
   Name texture{};
   Name normal_texture{};
   float diffuseAmount{};
};

}