#include <utility>

#include "widget/TextBox.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

#include "triglav/render_core/GlyphCache.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_position(const Vector4 area, const Vector2 size, const TextAlignment alignment)
{
   switch (alignment) {
   case TextAlignment::Left:
      return {area.x, area.y + size.y};
   case TextAlignment::Center:
      return {area.x + 0.5f * area.z - 0.5f * size.x, area.y + size.y};
   case TextAlignment::Right:
      return {area.x + area.z - size.x, area.y + size.y};
   }

   return {area.x, area.y};
}

}// namespace

TextBox::TextBox(Context& ctx, State initialState, IWidget* parent) :
    m_uiContext(ctx),
    m_state(std::move(initialState)),
    m_parent(parent)
{
}

Vector2 TextBox::desired_size(const Vector2 /*parentSize*/) const
{
   if (m_cachedDesiredSize.has_value()) {
      return *m_cachedDesiredSize;
   }

   m_cachedDesiredSize.emplace(this->calculate_desired_size());
   return *m_cachedDesiredSize;
}

void TextBox::add_to_viewport(const Vector4 dimensions)
{
   const auto size = this->desired_size({});

   if (m_id != 0) {
      m_uiContext.viewport().set_text_position(m_id, calculate_position(dimensions, size, m_state.alignment));
      return;
   }

   Text text{
      .content = m_state.content,
      .typefaceName = m_state.typeface,
      .fontSize = m_state.fontSize,
      .position = calculate_position(dimensions, size, m_state.alignment),
      .color = m_state.color,
   };

   m_id = m_uiContext.viewport().add_text(std::move(text));
}

void TextBox::remove_from_viewport()
{
   m_uiContext.viewport().remove_text(m_id);
   m_id = 0;
}

void TextBox::set_content(const std::string_view content)
{
   if (m_state.content == content)
      return;
   if (m_id == 0)
      return;

   m_state.content = content;
   m_uiContext.viewport().set_text_content(m_id, content);

   const auto newDesiredSize = this->calculate_desired_size();
   if (m_cachedDesiredSize.has_value() && *m_cachedDesiredSize == newDesiredSize) {
      return;
   }

   m_cachedDesiredSize.emplace(newDesiredSize);

   if (m_parent != nullptr) {
      m_parent->on_child_state_changed(*this);
   }
}

Vector2 TextBox::calculate_desired_size() const
{
   auto& glyphAtlas = m_uiContext.glyph_cache().find_glyph_atlas({m_state.typeface, m_state.fontSize});
   const auto measure = glyphAtlas.measure_text(m_state.content);
   const auto measureZeroChar = glyphAtlas.measure_text("0");
   return {measure.width, measureZeroChar.height};
}

}// namespace triglav::ui_core