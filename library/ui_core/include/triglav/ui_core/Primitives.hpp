#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/String.hpp"

namespace triglav::ui_core {

struct Text
{
   String content;
   TypefaceName typefaceName;
   i32 fontSize{};
   Vector2 position;
   Vector4 color;
   Vector4 crop;
};

struct Rectangle
{
   Vector4 rect;
   Vector4 color;
   Vector4 borderRadius;
   Vector4 borderColor;
   float borderWidth;
};

struct Sprite
{
   TextureName texture;
   Vector2 position;
   Vector2 size;
   std::optional<Vector4> textureRegion;
};

}// namespace triglav::ui_core