#pragma once

#include "Primitives.hpp"

#include "triglav/Delegate.hpp"

#include <map>

namespace triglav::ui_core {

class Viewport {
 public:
   using OnAddedTextDel = Delegate<Name, const Text&>;
   using OnTextChangeContentDel = Delegate<Name, const Text&>;
   using OnAddedRectangleDel = Delegate<Name, const Rectangle&>;

   OnAddedTextDel OnAddedText;
   OnTextChangeContentDel OnTextChangeContent;
   OnAddedRectangleDel OnAddedRectangle;

   void add_text(Name name, Text&& text);
   void set_text_content(Name name, std::string_view content);

   void add_rectangle(Name name, Rectangle&& rect);

   [[nodiscard]] const Text& text(Name name) const;

 private:
   std::map<Name, Text> m_texts;
   std::map<Name, Rectangle> m_rectangles;
};

}