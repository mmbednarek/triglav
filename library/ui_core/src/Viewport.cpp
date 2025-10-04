#include "Viewport.hpp"

#include "Primitives.hpp"

#include <utility>
#include <spdlog/spdlog.h>

namespace triglav::ui_core {

Viewport::Viewport(const Vector2u dimensions) :
    m_dimensions(dimensions)
{
}

Name Viewport::add_text(Text&& text)
{
   const Name result = m_topName;
   ++m_topName;
   this->add_text(result, std::move(text));
   return result;
}

void Viewport::add_text(const Name name, Text&& text)
{
   auto [it, added] = m_texts.emplace(name, std::move(text));
   assert(added);

   this->event_OnAddedText.publish(name, it->second);
   m_needsRedraw = true;
}

void Viewport::set_text_content(const Name name, const StringView content)
{
   auto& textPrim = m_texts.at(name);

   if (textPrim.content == content)
      return;
   textPrim.content = content;

   this->event_OnTextChangeContent.publish(name, textPrim);
   m_needsRedraw = true;
}

void Viewport::set_text_position(Name name, const Vector2 position)
{
   auto& textPrim = m_texts.at(name);

   if (textPrim.position == position)
      return;
   textPrim.position = position;

   spdlog::info("ui-viewport: text: {} = (position: {}, {})", name, position.x, position.y);

   this->event_OnTextChangePosition.publish(name, textPrim);
   m_needsRedraw = true;
}

void Viewport::set_text_crop(Name name, const Vector4 crop)
{
   auto& textPrim = m_texts.at(name);

   if (textPrim.crop == crop)
      return;
   textPrim.crop = crop;

   spdlog::info("ui-viewport: text: {} = (crop: {}, {}, {}, {})", name, crop.x, crop.y, crop.z, crop.w);

   this->event_OnTextChangeCrop.publish(name, textPrim);
   m_needsRedraw = true;
}

void Viewport::set_text_color(const Name name, const Vector4 color)
{
   auto& textPrim = m_texts.at(name);

   if (textPrim.color == color)
      return;
   textPrim.color = color;

   spdlog::info("ui-viewport: text: {} = (color: {}, {}, {}, {})", name, color.x, color.y, color.z, color.w);

   this->event_OnTextChangeColor.publish(name, textPrim);
   m_needsRedraw = true;
}

void Viewport::remove_text(const Name name)
{
   spdlog::info("ui-viewport: removing text: {}", name);
   this->event_OnRemovedText.publish(name);
   m_texts.erase(name);
   m_needsRedraw = true;
}

void Viewport::add_rectangle(Name name, Rectangle&& rect)
{
   auto [it, added] = m_rectangles.emplace(name, std::move(rect));
   assert(added);

   this->event_OnAddedRectangle.publish(name, it->second);
   m_needsRedraw = true;
}

Name Viewport::add_rectangle(Rectangle&& rect)
{
   const auto name = m_topName;
   ++m_topName;
   this->add_rectangle(name, std::move(rect));
   return name;
}

void Viewport::set_rectangle_dims(const Name name, const Vector4 dims)
{
   auto& rect = m_rectangles.at(name);
   if (rect.rect == dims)
      return;

   spdlog::info("ui-viewport: rect: {} = (dims: {}, {}, {}, {})", name, dims.x, dims.y, dims.z, dims.w);

   rect.rect = dims;
   this->event_OnRectangleChangeDims.publish(name, rect);
   m_needsRedraw = true;
}

void Viewport::set_rectangle_dims_with_crop(Name name, const Vector4 dims, const Vector4 crop)
{
   auto& rect = m_rectangles.at(name);
   if (rect.rect == dims)
      return;

   rect.rect = dims;
   rect.crop = crop;
   spdlog::info("ui-viewport: rect: {} = (dims: {}, {}, {}, {}, crop: {}, {}, {}, {})", name, dims.x, dims.y, dims.z, dims.w, crop.x, crop.y, crop.z, crop.w);
   this->event_OnRectangleChangeDims.publish(name, rect);
   m_needsRedraw = true;
}

void Viewport::set_rectangle_color(Name name, const Vector4 color)
{
   auto& rect = m_rectangles.at(name);
   if (rect.color == color)
      return;

   spdlog::info("ui-viewport: rect: {} = (color: {}, {}, {}, {})", name, color.x, color.y, color.z, color.w);

   rect.color = color;
   this->event_OnRectangleChangeColor.publish(name, rect);
   m_needsRedraw = true;
}

void Viewport::remove_rectangle(Name name)
{
   spdlog::info("ui-viewport: removing rect: {}", name);

   event_OnRemovedRectangle.publish(name);
   m_rectangles.erase(name);
   m_needsRedraw = true;
}

void Viewport::add_sprite(Name name, Sprite&& sprite)
{
   auto [it, added] = m_sprites.emplace(name, sprite);
   assert(added);
   this->event_OnAddedSprite.publish(name, it->second);
   m_needsRedraw = true;
}

Name Viewport::add_sprite(Sprite&& sprite)
{
   const auto name = m_topName;
   ++m_topName;
   this->add_sprite(name, std::move(sprite));
   return name;
}

void Viewport::set_sprite_position(const Name name, const Vector2 position)
{
   auto& sprite = m_sprites.at(name);
   sprite.position = position;
   event_OnSpriteChangePosition.publish(name, sprite);
   m_needsRedraw = true;
}

void Viewport::set_sprite_texture_region(Name name, Vector4 region)
{
   auto& sprite = m_sprites.at(name);
   sprite.textureRegion = region;
   event_OnSpriteChangeTextureRegion.publish(name, sprite);
   m_needsRedraw = true;
}

void Viewport::remove_sprite(Name name)
{
   event_OnRemovedSprite.publish(name);
   m_sprites.erase(name);
   m_needsRedraw = true;
}

const Text& Viewport::text(const Name name) const
{
   return m_texts.at(name);
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
