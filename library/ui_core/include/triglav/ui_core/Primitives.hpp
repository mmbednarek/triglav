#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"

#include <string>

namespace triglav::ui_core {

struct Text
{
   std::string content;
   TypefaceName typefaceName;
   i32 fontSize{};
   Vector2 position;
   Vector4 color;
};

struct Rectangle
{
   Vector4 rect;
   Vector4 color;
};

struct Sprite
{
   TextureName texture;
   Vector2 position;
   Vector2 size;
   std::optional<Vector4> textureRegion;
};

}// namespace triglav::ui_core