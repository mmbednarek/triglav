#pragma once

#include "Name.hpp"

namespace renderer {

struct Material
{
   Name texture{};
   float diffuseAmount{};
};

}