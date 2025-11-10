#include <utility>

#include "widget/TextBox.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

#include "triglav/render_core/GlyphCache.hpp"

namespace triglav::ui_core {

using namespace string_literals;

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

void TextBox::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   if (!do_regions_intersect(dimensions, croppingMask)) {
      if (m_id != 0) {
         m_uiContext.viewport().remove_text(m_id);
         m_id = 0;
      }
      return;
   }

   const auto size = this->desired_size({});
   const Vector2 position{
      dimensions.x + calculate_alignment(m_state.horizontalAlignment, dimensions.z, size.x),
      dimensions.y + size.y + calculate_alignment(m_state.verticalAlignment, dimensions.w, size.y),
   };


   if (m_id != 0) {
      m_uiContext.viewport().set_text_position(m_id, position, croppingMask);
      return;
   }

   Text text{
      .content = m_state.content,
      .typefaceName = m_state.typeface,
      .fontSize = m_state.fontSize,
      .position = position,
      .color = m_state.color,
      .crop = croppingMask,
   };

   m_id = m_uiContext.viewport().add_text(std::move(text));
}

void TextBox::remove_from_viewport()
{
   m_uiContext.viewport().remove_text(m_id);
   m_id = 0;
}

void TextBox::set_content(const StringView content)
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

void TextBox::set_color(const Vector4 color)
{
   if (m_state.color == color)
      return;
   if (m_id == 0)
      return;

   m_state.color = color;
   m_uiContext.viewport().set_text_color(m_id, color);
}

const TextBox::State& TextBox::state() const
{
   return m_state;
}

Vector2 TextBox::calculate_desired_size() const
{
   auto& glyphAtlas = m_uiContext.glyph_cache().find_glyph_atlas({m_state.typeface, m_state.fontSize});
   const auto measure = glyphAtlas.measure_text(m_state.content.view());
   const auto measureZeroChar = glyphAtlas.measure_text("0"_strv);
   return {measure.width, measureZeroChar.height};
}

}// namespace triglav::ui_core