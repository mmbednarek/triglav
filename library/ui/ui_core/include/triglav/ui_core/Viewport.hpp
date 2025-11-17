#pragma once

#include "Primitives.hpp"

#include "triglav/Logging.hpp"
#include "triglav/Math.hpp"
#include "triglav/event/Delegate.hpp"

#include <map>

namespace triglav::ui_core {

class Viewport
{
   TG_DEFINE_LOG_CATEGORY(UI_Viewport)
 public:
   TG_EVENT(OnAddedText, TextId, const Text&)
   TG_EVENT(OnUpdatedText, TextId, const Text&)
   TG_EVENT(OnRemovedText, TextId)

   TG_EVENT(OnAddedRectangle, RectId, const Rectangle&)
   TG_EVENT(OnUpdatedRectangle, RectId, const Rectangle&)
   TG_EVENT(OnRemovedRectangle, RectId)

   TG_EVENT(OnAddedSprite, SpriteId, const Sprite&)
   TG_EVENT(OnUpdatedSprite, SpriteId, const Sprite&)
   TG_EVENT(OnRemovedSprite, SpriteId)

   explicit Viewport(Vector2u dimensions);

   TextId add_text(Text&& text);
   void set_text_content(TextId text_id, StringView content);
   void set_text_position(TextId text_id, Vector2 position, Rect crop);
   void set_text_color(TextId text_id, Color color);
   void remove_text(TextId text_id);

   RectId add_rectangle(Rectangle&& rect);
   void set_rectangle_dims(RectId rect_id, Rect dims, Rect crop);
   void set_rectangle_color(RectId rect_id, Color color);
   void remove_rectangle(RectId rect_id);
   void remove_rectangle_safe(RectId& rect_id);

   SpriteId add_sprite(Sprite&& sprite);
   void set_sprite_position(SpriteId sprite_id, Vector2 position, Rect crop);
   void set_sprite_texture_region(SpriteId sprite_id, Rect region);
   void remove_sprite(SpriteId sprite_id);

   [[nodiscard]] const Text& text(TextId text_id) const;
   [[nodiscard]] Vector2u dimensions() const;
   void set_dimensions(Vector2u dimensions);

   [[nodiscard]] bool should_redraw();

 private:
   TextId m_top_text_id{1};
   RectId m_top_rect_id{1};
   SpriteId m_top_sprite_id{1};
   std::map<Name, Text> m_texts;
   std::map<Name, Rectangle> m_rectangles;
   std::map<Name, Sprite> m_sprites;
   Vector2u m_dimensions{};
   bool m_needs_redraw{true};
};

}// namespace triglav::ui_core