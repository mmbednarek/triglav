#include "Viewport.hpp"

#include "Primitives.hpp"

#include <utility>
#include <spdlog/spdlog.h>

namespace triglav::ui_core {

Viewport::Viewport(const Vector2u dimensions) :
    m_dimensions(dimensions)
{
}

TextId Viewport::add_text(Text&& text)
{
   auto textId = m_topTextId++;
   auto [it, added] = m_texts.emplace(textId, std::move(text));
   assert(added);

   spdlog::info("ui-viewport: add text: {}", textId);

   this->event_OnAddedText.publish(textId, it->second);
   m_needsRedraw = true;

   return textId;
}

void Viewport::set_text_content(const TextId textId, const StringView content)
{
   auto& textPrim = m_texts.at(textId);

   if (textPrim.content == content)
      return;
   textPrim.content = content;

   this->event_OnUpdatedText.publish(textId, textPrim);
   m_needsRedraw = true;
}

void Viewport::set_text_position(TextId textId, const Vector2 position, const Rect crop)
{
   auto& textPrim = m_texts.at(textId);

   if (textPrim.position == position && textPrim.crop == crop)
      return;
   textPrim.position = position;
   textPrim.crop = crop;

   // spdlog::info("ui-viewport: text: {} = (position: {}, {}, crop: {}, {}, {}, {})", textId, position.x, position.y, crop.x, crop.y, crop.z, crop.w);

   this->event_OnUpdatedText.publish(textId, textPrim);
   m_needsRedraw = true;
}

void Viewport::set_text_color(const TextId textId, const Color color)
{
   auto& textPrim = m_texts.at(textId);

   if (textPrim.color == color)
      return;
   textPrim.color = color;

   // spdlog::info("ui-viewport: text: {} = (color: {}, {}, {}, {})", textId, color.x, color.y, color.z, color.w);

   this->event_OnUpdatedText.publish(textId, textPrim);
   m_needsRedraw = true;
}

void Viewport::remove_text(const TextId textId)
{
   spdlog::info("ui-viewport: removing text: {}", textId);
   this->event_OnRemovedText.publish(textId);
   m_texts.erase(textId);
   m_needsRedraw = true;
}

RectId Viewport::add_rectangle(Rectangle&& rect)
{
   auto rectId = m_topRectId++;
   auto [it, added] = m_rectangles.emplace(rectId, rect);
   assert(added);

   spdlog::info("ui-viewport: add rectangle: {}", rectId);

   this->event_OnAddedRectangle.publish(rectId, it->second);
   m_needsRedraw = true;

   return rectId;
}

void Viewport::set_rectangle_dims(const RectId rectId, const Rect dims, const Rect crop)
{
   auto& rect = m_rectangles.at(rectId);
   if (rect.rect == dims && rect.crop == crop)
      return;

   // spdlog::info("ui-viewport: rect: {} = (dims: {}, {}, {}, {}, crop: {}, {}, {}, {})", rectId, dims.x, dims.y, dims.z, dims.w, crop.x, crop.y, crop.z, crop.w);

   rect.rect = dims;
   rect.crop = crop;
   this->event_OnUpdatedRectangle.publish(rectId, rect);
   m_needsRedraw = true;
}

void Viewport::set_rectangle_color(RectId rectId, const Vector4 color)
{
   auto& rect = m_rectangles.at(rectId);
   if (rect.color == color)
      return;

   // spdlog::info("ui-viewport: rect: {} = (color: {}, {}, {}, {})", rectId, color.x, color.y, color.z, color.w);

   rect.color = color;
   this->event_OnUpdatedRectangle.publish(rectId, rect);
   m_needsRedraw = true;
}

void Viewport::remove_rectangle(RectId rectId)
{
   spdlog::info("ui-viewport: removing rect: {}", rectId);

   event_OnRemovedRectangle.publish(rectId);
   m_rectangles.erase(rectId);
   m_needsRedraw = true;
}

SpriteId Viewport::add_sprite(Sprite&& sprite)
{
   auto spriteId = m_topSpriteId++;
   auto [it, added] = m_sprites.emplace(spriteId, sprite);
   assert(added);
   this->event_OnAddedSprite.publish(spriteId, it->second);
   m_needsRedraw = true;
   return spriteId;
}

void Viewport::set_sprite_position(const SpriteId spriteId, const Vector2 position, const Rect /*crop*/)
{
   auto& sprite = m_sprites.at(spriteId);
   sprite.position = position;
   event_OnUpdatedSprite.publish(spriteId, sprite);
   m_needsRedraw = true;
}

void Viewport::set_sprite_texture_region(const SpriteId spriteId, Vector4 region)
{
   auto& sprite = m_sprites.at(spriteId);
   sprite.textureRegion = region;
   event_OnUpdatedSprite.publish(spriteId, sprite);
   m_needsRedraw = true;
}

void Viewport::remove_sprite(const SpriteId spriteId)
{
   event_OnRemovedSprite.publish(spriteId);
   m_sprites.erase(spriteId);
   m_needsRedraw = true;
}

const Text& Viewport::text(const TextId textId) const
{
   return m_texts.at(textId);
}

Vector2u Viewport::dimensions() const
{
   return m_dimensions;
}

bool Viewport::should_redraw()
{
   if (m_needsRedraw) {
      m_needsRedraw = false;
      return true;
   }
   return false;
}

}// namespace triglav::ui_core
