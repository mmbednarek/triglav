#include "PrimitiveHelpers.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

namespace triglav::ui_core {

void TextInstance::add(Context& ctx, Vector2 position, Vector4 crop)
{
   if (this->text_id == 0) {
      this->text_id = ctx.viewport().add_text(Text{
         .content = this->content,
         .typeface_name = this->typeface_name,
         .font_size = this->font_size,
         .position = position,
         .color = this->color,
         .crop = crop,
      });

      return;
   }

   ctx.viewport().set_text_position(this->text_id, position, crop);
}

void TextInstance::set_content(Context& ctx, StringView new_content)
{
   this->content = new_content;
   if (this->text_id != 0) {
      ctx.viewport().set_text_content(this->text_id, new_content);
   }
}

void TextInstance::remove(Context& ctx)
{
   if (this->text_id != 0) {
      ctx.viewport().remove_text(this->text_id);
      this->text_id = 0;
   }
}

void RectInstance::add(Context& ctx, Vector4 dimensions, Vector4 crop)
{
   if (rect_id == 0) {
      rect_id = ctx.viewport().add_rectangle(Rectangle{
         .rect = dimensions,
         .color = this->color,
         .border_radius = this->border_radius,
         .border_color = this->border_color,
         .crop = crop,
         .border_width = this->border_width,
      });

      return;
   }

   ctx.viewport().set_rectangle_dims(rect_id, dimensions, crop);
}

void RectInstance::remove(Context& ctx)
{
   if (rect_id != 0) {
      ctx.viewport().remove_rectangle(rect_id);
      rect_id = 0;
   }
}

void RectInstance::set_color(Context& ctx, Color new_color)
{
   this->color = new_color;
   if (rect_id != 0) {
      ctx.viewport().set_rectangle_color(rect_id, new_color);
   }
}

void SpriteInstance::add(Context& ctx, Vector4 dimensions, Vector4 crop)
{
   if (sprite_id == 0) {
      sprite_id = ctx.viewport().add_sprite({
         .texture = this->texture,
         .position = rect_position(dimensions),
         .size = rect_size(dimensions),
         .crop = crop,
         .texture_region = this->texture_region,
      });
      return;
   }

   ctx.viewport().set_sprite_position(sprite_id, rect_position(dimensions), crop);
}

void SpriteInstance::remove(Context& ctx)
{
   if (sprite_id != 0) {
      ctx.viewport().remove_sprite(sprite_id);
      sprite_id = 0;
   }
}

void SpriteInstance::set_region(Context& ctx, Vector4 region)
{
   this->texture_region = region;
   if (sprite_id != 0) {
      ctx.viewport().set_sprite_texture_region(sprite_id, region);
   }
}

}// namespace triglav::ui_core
