#pragma once

#include "triglav/Name.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>

namespace triglav::ui_core {

struct Text
{
   std::string content;
   TypefaceName typefaceName;
   int fontSize{};
   glm::vec2 position;
   glm::vec4 color;
};

struct Rectangle
{
   glm::vec4 rect;
   glm::vec4 color;
};

}// namespace triglav::ui_core