#pragma once

#include "Primitives.hpp"

#include "triglav/Math.hpp"
#include "triglav/event/Delegate.hpp"

#include <map>

namespace triglav::ui_core {

class Viewport
{
 public:
   TG_EVENT(OnAddedText, Name, const Text&)
   TG_EVENT(OnRemovedText, Name)
   TG_EVENT(OnTextChangeContent, Name, const Text&)
   TG_EVENT(OnTextChangePosition, Name, const Text&)
   TG_EVENT(OnTextChangeCrop, Name, const Text&)
   TG_EVENT(OnTextChangeColor, Name, const Text&)

   TG_EVENT(OnAddedRectangle, Name, const Rectangle&)
   TG_EVENT(OnRectangleChangeDims, Name, const Rectangle&)
   TG_EVENT(OnRectangleChangeColor, Name, const Rectangle&)
   TG_EVENT(OnRemovedRectangle, Name)

   TG_EVENT(OnAddedSprite, Name, const Sprite&)
   TG_EVENT(OnSpriteChangePosition, Name, const Sprite&)
   TG_EVENT(OnSpriteChangeTextureRegion, Name, const Sprite&)
   TG_EVENT(OnRemovedSprite, Name)

   explicit Viewport(Vector2u dimensions);

   Name add_text(Text&& text);
   void add_text(Name name, Text&& text);
   void set_text_content(Name name, StringView content);
   void set_text_position(Name name, Vector2 position);
   void set_text_crop(Name name, Vector4 crop);
   void set_text_color(Name name, Vector4 color);
   void set_rectangle_dims(Name name, Vector4 dims);
   void remove_text(Name name);

   void add_rectangle(Name name, Rectangle&& rect);
   Name add_rectangle(Rectangle&& rect);
   void set_rectangle_color(Name name, Vector4 color);
   void remove_rectangle(Name name);

   void add_sprite(Name name, Sprite&& sprite);
   Name add_sprite(Sprite&& sprite);
   void set_sprite_position(Name name, Vector2 position);
   void set_sprite_texture_region(Name name, Vector4 region);
   void remove_sprite(Name name);

   [[nodiscard]] const Text& text(Name name) const;
   [[nodiscard]] Vector2u dimensions() const;

   [[nodiscard]] bool should_redraw();

 private:
   Name m_topName{1};
   std::map<Name, Text> m_texts;
   std::map<Name, Rectangle> m_rectangles;
   std::map<Name, Sprite> m_sprites;
   Vector2u m_dimensions{};
   bool m_needsRedraw{true};
};

}// namespace triglav::ui_core