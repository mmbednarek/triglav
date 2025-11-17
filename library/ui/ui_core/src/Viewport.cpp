#include "Viewport.hpp"

#include "Primitives.hpp"

#include <utility>

namespace triglav::ui_core {

Viewport::Viewport(const Vector2u dimensions) :
    m_dimensions(dimensions)
{
}

TextId Viewport::add_text(Text&& text)
{
   auto text_id = m_top_text_id++;
   auto [it, added] = m_texts.emplace(text_id, std::move(text));
   assert(added);

   log_debug("Add text: {}", text_id);

   this->event_OnAddedText.publish(text_id, it->second);
   m_needs_redraw = true;

   return text_id;
}

void Viewport::set_text_content(const TextId text_id, const StringView content)
{
   auto& text_prim = m_texts.at(text_id);

   if (text_prim.content == content)
      return;
   text_prim.content = content;

   this->event_OnUpdatedText.publish(text_id, text_prim);
   m_needs_redraw = true;
}

void Viewport::set_text_position(TextId text_id, const Vector2 position, const Rect crop)
{
   auto& text_prim = m_texts.at(text_id);

   if (text_prim.position == position && text_prim.crop == crop)
      return;
   text_prim.position = position;
   text_prim.crop = crop;

   // log_debug("text: {} = (position: {}, {}, crop: {}, {}, {}, {})", text_id, position.x, position.y, crop.x, crop.y,
   // crop.z, crop.w);

   this->event_OnUpdatedText.publish(text_id, text_prim);
   m_needs_redraw = true;
}

void Viewport::set_text_color(const TextId text_id, const Color color)
{
   auto& text_prim = m_texts.at(text_id);

   if (text_prim.color == color)
      return;
   text_prim.color = color;

   // log_debug("text: {} = (color: {}, {}, {}, {})", text_id, color.x, color.y, color.z, color.w);

   this->event_OnUpdatedText.publish(text_id, text_prim);
   m_needs_redraw = true;
}

void Viewport::remove_text(const TextId text_id)
{
   log_debug("Removing text: {}", text_id);
   this->event_OnRemovedText.publish(text_id);
   m_texts.erase(text_id);
   m_needs_redraw = true;
}

RectId Viewport::add_rectangle(Rectangle&& rect)
{
   auto rect_id = m_top_rect_id++;
   auto [it, added] = m_rectangles.emplace(rect_id, rect);
   assert(added);

   log_debug("Add rectangle: {}", rect_id);

   this->event_OnAddedRectangle.publish(rect_id, it->second);
   m_needs_redraw = true;

   return rect_id;
}

void Viewport::set_rectangle_dims(const RectId rect_id, const Rect dims, const Rect crop)
{
   auto& rect = m_rectangles.at(rect_id);
   if (rect.rect == dims && rect.crop == crop)
      return;

   log_debug("rect: {} = (dims: {}, crop: {})", rect_id, dims, crop);

   rect.rect = dims;
   rect.crop = crop;
   this->event_OnUpdatedRectangle.publish(rect_id, rect);
   m_needs_redraw = true;
}

void Viewport::set_rectangle_color(RectId rect_id, const Vector4 color)
{
   auto& rect = m_rectangles.at(rect_id);
   if (rect.color == color)
      return;

   // log_debug("rect: {} = (color: {}, {}, {}, {})", rect_id, color.x, color.y, color.z, color.w);

   rect.color = color;
   this->event_OnUpdatedRectangle.publish(rect_id, rect);
   m_needs_redraw = true;
}

void Viewport::remove_rectangle(RectId rect_id)
{
   assert(rect_id != 0);
   log_debug("Removing rect: {}", rect_id);

   event_OnRemovedRectangle.publish(rect_id);
   m_rectangles.erase(rect_id);
   m_needs_redraw = true;
}

void Viewport::remove_rectangle_safe(RectId& rect_id)
{
   if (rect_id == 0)
      return;
   this->remove_rectangle(rect_id);
   rect_id = 0;
}

SpriteId Viewport::add_sprite(Sprite&& sprite)
{
   auto sprite_id = m_top_sprite_id++;
   auto [it, added] = m_sprites.emplace(sprite_id, sprite);
   assert(added);
   this->event_OnAddedSprite.publish(sprite_id, it->second);
   m_needs_redraw = true;
   return sprite_id;
}

void Viewport::set_sprite_position(const SpriteId sprite_id, const Vector2 position, const Rect crop)
{
   auto& sprite = m_sprites.at(sprite_id);
   sprite.position = position;
   sprite.crop = crop;
   event_OnUpdatedSprite.publish(sprite_id, sprite);
   m_needs_redraw = true;
}

void Viewport::set_sprite_texture_region(const SpriteId sprite_id, Vector4 region)
{
   auto& sprite = m_sprites.at(sprite_id);
   sprite.texture_region = region;
   event_OnUpdatedSprite.publish(sprite_id, sprite);
   m_needs_redraw = true;
}

void Viewport::remove_sprite(const SpriteId sprite_id)
{
   event_OnRemovedSprite.publish(sprite_id);
   m_sprites.erase(sprite_id);
   m_needs_redraw = true;
}

const Text& Viewport::text(const TextId text_id) const
{
   return m_texts.at(text_id);
}

Vector2u Viewport::dimensions() const
{
   return m_dimensions;
}

void Viewport::set_dimensions(Vector2u dimensions)
{
   m_dimensions = dimensions;
}

bool Viewport::should_redraw()
{
   if (m_needs_redraw) {
      m_needs_redraw = false;
      return true;
   }
   return false;
}

}// namespace triglav::ui_core
