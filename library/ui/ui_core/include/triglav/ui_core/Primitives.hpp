#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/String.hpp"

#include <optional>

namespace triglav::ui_core {

using TextId = u32;
using RectId = u32;
using SpriteId = u32;

struct Text
{
   String content;
   TypefaceName typeface_name;
   i32 font_size{};
   Vector2 position;
   Vector4 color;
   Vector4 crop;
};

struct Rectangle
{
   Vector4 rect;
   Vector4 color;
   Vector4 border_radius;
   Vector4 border_color;
   Vector4 crop;
   float border_width;
};

struct Sprite
{
   TextureName texture;
   Vector2 position;
   Vector2 size;
   Vector4 crop;
   std::optional<Vector4> texture_region;
};

}// namespace triglav::ui_core