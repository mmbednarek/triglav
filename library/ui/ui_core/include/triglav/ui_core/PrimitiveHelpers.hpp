#pragma once

#include "Primitives.hpp"

namespace triglav::ui_core {

class Context;

struct TextInstance
{
   String content;
   TypefaceName typeface_name;
   i32 font_size{};
   Vector4 color;

   TextId text_id = 0;

   void add(Context& ctx, Vector2 position, Vector4 crop);
   void set_content(Context& ctx, StringView content);
   void remove(Context& ctx);
};

struct RectInstance
{
   Vector4 color{};
   Vector4 border_radius{};
   Vector4 border_color{};
   float border_width{};

   RectId rect_id = 0;

   void add(Context& ctx, Vector4 dimensions, Vector4 crop);
   void remove(Context& ctx);
   void set_color(Context& ctx, Color Color);
};

struct SpriteInstance
{
   TextureName texture;
   Vector2 size;
   std::optional<Vector4> texture_region;

   SpriteId sprite_id = 0;

   void add(Context& ctx, Vector4 dimensions, Vector4 crop);
   void remove(Context& ctx);
   void set_region(Context& ctx, Vector4 region);
};

}// namespace triglav::ui_core
