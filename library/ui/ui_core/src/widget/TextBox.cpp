#include <utility>

#include "widget/TextBox.hpp"

#include "Context.hpp"
#include "Viewport.hpp"

#include "triglav/render_core/GlyphCache.hpp"

namespace triglav::ui_core {

using namespace string_literals;

TextBox::TextBox(Context& ctx, State initial_state, IWidget* parent) :
    m_ui_context(ctx),
    m_state(std::move(initial_state)),
    m_parent(parent)
{
}

Vector2 TextBox::desired_size(const Vector2 /*available_size*/) const
{
   if (m_cached_desired_size.has_value()) {
      return *m_cached_desired_size;
   }

   m_cached_desired_size.emplace(this->calculate_desired_size());
   return *m_cached_desired_size;
}

void TextBox::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   if (!do_regions_intersect(dimensions, cropping_mask)) {
      if (m_id != 0) {
         m_ui_context.viewport().remove_text(m_id);
         m_id = 0;
      }
      return;
   }

   const auto size = this->desired_size({});
   const Vector2 position{
      dimensions.x + calculate_alignment(m_state.horizontal_alignment, dimensions.z, size.x),
      dimensions.y + size.y + calculate_alignment(m_state.vertical_alignment, dimensions.w, size.y),
   };


   if (m_id != 0) {
      m_ui_context.viewport().set_text_position(m_id, position, cropping_mask);
      return;
   }

   Text text{
      .content = m_state.content,
      .typeface_name = m_state.typeface,
      .font_size = m_state.font_size,
      .position = position,
      .color = m_state.color,
      .crop = cropping_mask,
   };

   m_id = m_ui_context.viewport().add_text(std::move(text));
}

void TextBox::remove_from_viewport()
{
   if (m_id != 0) {
      m_ui_context.viewport().remove_text(m_id);
      m_id = 0;
   }
}

void TextBox::set_content(const StringView content)
{
   if (m_state.content == content)
      return;
   if (m_id == 0)
      return;

   m_state.content = content;
   m_ui_context.viewport().set_text_content(m_id, content);

   const auto new_desired_size = this->calculate_desired_size();
   if (m_cached_desired_size.has_value() && *m_cached_desired_size == new_desired_size) {
      return;
   }

   m_cached_desired_size.emplace(new_desired_size);

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
   m_ui_context.viewport().set_text_color(m_id, color);
}

const TextBox::State& TextBox::state() const
{
   return m_state;
}

Vector2 TextBox::calculate_desired_size() const
{
   auto& glyph_atlas = m_ui_context.glyph_cache().find_glyph_atlas({m_state.typeface, m_state.font_size});
   const auto measure = glyph_atlas.measure_text(m_state.content.view());
   const auto measure_zero_char = glyph_atlas.measure_text("0"_strv);
   return {measure.width, measure_zero_char.height};
}

}// namespace triglav::ui_core