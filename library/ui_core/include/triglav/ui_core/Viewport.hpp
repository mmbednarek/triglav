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
   TG_EVENT(OnTextChangeContent, Name, const Text&)
   TG_EVENT(OnAddedRectangle, Name, const Rectangle&)
   TG_EVENT(OnRectangleChangeDims, Name, const Rectangle&)

   explicit Viewport(Vector2u dimensions);

   void add_text(Name name, Text&& text);
   void set_text_content(Name name, std::string_view content);
   void set_rectangle_dims(Name name, Vector4 dims);

   void add_rectangle(Name name, Rectangle&& rect);

   [[nodiscard]] const Text& text(Name name) const;
   [[nodiscard]] Vector2u dimensions() const;

 private:
   std::map<Name, Text> m_texts;
   std::map<Name, Rectangle> m_rectangles;
   Vector2u m_dimensions{};
};

}// namespace triglav::ui_core