#pragma once

#include "triglav/Name.hpp"

#include <string>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace triglav::ui_core {

struct Text {
   std::string content;
   GlyphAtlasName glyphAtlas;
   glm::vec2 position;
   glm::vec4 color;
};

struct Rectangle {
   glm::vec4 rect;
};

}