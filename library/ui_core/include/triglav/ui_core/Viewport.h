#pragma once

#include "Primitives.hpp"

#include "triglav/Delegate.hpp"

#include <map>

namespace triglav::ui_core {

class Viewport {
 public:
   using OnAddedTextDel = Delegate<NameID, const Text&>;
   using OnTextChangeContentDel = Delegate<NameID, const Text&>;
   using OnAddedRectangleDel = Delegate<NameID, const Rectangle&>;

   OnAddedTextDel OnAddedText;
   OnTextChangeContentDel OnTextChangeContent;
   OnAddedRectangleDel OnAddedRectangle;

   void add_text(NameID name, Text&& text);
   void set_text_content(NameID name, std::string_view content);

   void add_rectangle(NameID name, Rectangle&& rect);

   [[nodiscard]] const Text& text(NameID name) const;

 private:
   std::map<NameID, Text> m_texts;
   std::map<NameID, Rectangle> m_rectangles;
};

}