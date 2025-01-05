#pragma once

#include "Primitives.hpp"

#include "triglav/event/Delegate.hpp"

#include <map>

namespace triglav::ui_core {

class Viewport
{
 public:
   TG_EVENT(OnAddedText, Name, const Text&)
   TG_EVENT(OnTextChangeContent, Name, const Text&)
   TG_EVENT(OnAddedRectangle, Name, const Rectangle&)

   void add_text(Name name, Text&& text);
   void set_text_content(Name name, std::string_view content);

   void add_rectangle(Name name, Rectangle&& rect);

   [[nodiscard]] const Text& text(Name name) const;

 private:
   std::map<Name, Text> m_texts;
   std::map<Name, Rectangle> m_rectangles;
};

}// namespace triglav::ui_core