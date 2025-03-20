#include <utility>

#include "widget/TextBox.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

#include "triglav/render_core/GlyphCache.hpp"

namespace triglav::ui_core {

TextBox::TextBox(Context& ctx, State initialState) :
    m_uiContext(ctx),
    m_state(std::move(initialState))
{
}

Vector2 TextBox::desired_size(const Vector2 /*parentSize*/) const
{
   if (m_cachedDesiredSize.has_value()) {
      return *m_cachedDesiredSize;
   }
   auto& glyphAtlas = m_uiContext.glyph_cache().find_glyph_atlas({m_state.typeface, m_state.fontSize});
   const auto measurement = glyphAtlas.measure_text(m_state.content);
   m_cachedDesiredSize.emplace(measurement.width, measurement.height);
   return *m_cachedDesiredSize;
}

void TextBox::add_to_viewport(const Vector4 dimensions)
{
   if (m_id != 0) {
      m_uiContext.viewport().set_text_position(m_id, dimensions);
      return;
   }

   const auto size = this->desired_size({});

   Text text{
      .content = m_state.content,
      .typefaceName = m_state.typeface,
      .fontSize = m_state.fontSize,
      .position = Vector2i{dimensions.x, dimensions.y + size.y},
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
   m_state.content = content;
   m_uiContext.viewport().set_text_content(m_id, content);
   m_cachedDesiredSize.reset();
}

}// namespace triglav::ui_core