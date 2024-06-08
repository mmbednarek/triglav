#include "Viewport.h"

#include "Primitives.hpp"

#include <utility>

namespace triglav::ui_core {

void Viewport::add_text(const Name name, Text&& text)
{
   auto [it, added] = m_texts.emplace(name, std::move(text));
   assert(added);

   this->OnAddedText.publish(name, it->second);
}

void Viewport::set_text_content(const Name name, const std::string_view content)
{
   auto& textPrim = m_texts.at(name);

   if (textPrim.content == content)
      return;
   textPrim.content = content;

   this->OnTextChangeContent.publish(name, textPrim);
}

void Viewport::add_rectangle(Name name, Rectangle&& rect)
{
   auto [it, added] = m_rectangles.emplace(name, std::move(rect));
   assert(added);

   this->OnAddedRectangle.publish(name, it->second);
}

const Text& Viewport::text(const Name name) const
{
   return m_texts.at(name);
}

}// namespace triglav::ui_core
