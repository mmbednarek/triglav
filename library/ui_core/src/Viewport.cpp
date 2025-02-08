#include "Viewport.hpp"

#include "Primitives.hpp"

#include <utility>

namespace triglav::ui_core {

Viewport::Viewport(const Vector2u dimensions) :
    m_dimensions(dimensions)
{
}

void Viewport::add_text(const Name name, Text&& text)
{
   auto [it, added] = m_texts.emplace(name, std::move(text));
   assert(added);

   this->event_OnAddedText.publish(name, it->second);
}

void Viewport::set_text_content(const Name name, const std::string_view content)
{
   auto& textPrim = m_texts.at(name);

   if (textPrim.content == content)
      return;
   textPrim.content = content;

   this->event_OnTextChangeContent.publish(name, textPrim);
}

void Viewport::set_rectangle_dims(const Name name, const Vector4 dims)
{
   auto& rect = m_rectangles.at(name);
   rect.rect = dims;

   this->event_OnRectangleChangeDims.publish(name, rect);
}

void Viewport::add_rectangle(Name name, Rectangle&& rect)
{
   auto [it, added] = m_rectangles.emplace(name, std::move(rect));
   assert(added);

   this->event_OnAddedRectangle.publish(name, it->second);
}

const Text& Viewport::text(const Name name) const
{
   return m_texts.at(name);
}

Vector2u Viewport::dimensions() const
{
   return m_dimensions;
}

}// namespace triglav::ui_core
