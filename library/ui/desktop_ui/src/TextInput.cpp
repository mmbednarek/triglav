#include "TextInput.hpp"

#include "triglav/desktop/ISurface.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/threading/Scheduler.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"

namespace triglav::desktop_ui {

using namespace std::chrono_literals;
using namespace string_literals;

constexpr Vector2 g_text_margin{8, 10};
constexpr float g_carret_margin{6};

TextInput::TextInput(ui_core::Context& ctx, const TextInput::State state, ui_core::IWidget* /*parent*/) :
    m_context(ctx),
    m_state(state),
    m_background_rect{
       .color = TG_THEME_VAL(text_input.bg_inactive),
       .border_radius = {8, 8, 8, 8},
       .border_color = m_state.border_color,
       .border_width = 1.0f,
    },
    m_selection_rect{
       .color = {0.5, 0.5, 0.5, 1.0},
       .border_radius = {0, 0, 0, 0},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    },
    m_text_prim{
       .content = m_state.text,
       .typeface_name = TG_THEME_VAL(base_typeface),
       .font_size = TG_THEME_VAL(button.font_size) - 1,
       .color = TG_THEME_VAL(foreground_color),
    },
    m_caret_box{
       .color = {0, 0, 0, 0},
       .border_radius = {},
       .border_color = {},
       .border_width = 0.0f,
    },
    m_caret_position(static_cast<u32>(m_state.text.rune_count()))
{
   const auto& props = m_state.manager->properties();

   auto& glyph_atlas = m_context.glyph_cache().find_glyph_atlas({props.base_typeface, props.button.font_size});
   const auto measure = glyph_atlas.measure_text("0"_strv);

   m_text_size = {measure.width, measure.height};
}

Vector2 TextInput::desired_size(const Vector2 parent_size) const
{
   return {parent_size.x, m_text_size.y + 2 * g_text_margin.y};
}

void TextInput::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_context.add_activating_widget(this);

   if (!do_regions_intersect(dimensions, cropping_mask)) {
      m_background_rect.remove(m_context);
      m_caret_box.remove(m_context);
      m_text_prim.remove(m_context);
      return;
   }

   m_text_offset = 0.0f;

   m_background_rect.add(m_context, dimensions, cropping_mask);

   this->update_selection_box();

   const Vector4 caret_dims{dimensions.x + g_text_margin.x, dimensions.y + g_carret_margin, 1, dimensions.w - 2 * g_carret_margin};
   m_caret_box.add(m_context, caret_dims, cropping_mask);

   m_text_xposition = dimensions.x + g_text_margin.x;
   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;
   this->update_text_position();
   this->recalculate_caret_offset();
}

void TextInput::remove_from_viewport()
{
   m_context.remove_activating_widget(this);

   m_background_rect.remove(m_context);
   m_selection_rect.remove(m_context);
   m_caret_box.remove(m_context);
   m_text_prim.remove(m_context);
}

void TextInput::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void TextInput::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   m_caret_position = this->rune_index_from_offset(event.mouse_position.x);
   this->on_activated(event);
   m_selected_count = 0;
   m_is_selecting = true;
   this->update_selection_box();
}

void TextInput::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   m_is_selecting = false;
}

void TextInput::on_mouse_moved(const ui_core::Event& event)
{
   if (!m_is_selecting)
      return;

   if (event.global_mouse_position.x < m_dimensions.x && m_text_offset < 0.0f) {
      m_text_offset = std::min(m_text_offset + 1.0f, 0.0f);
      this->update_text_position();
      this->update_selection_box();
   }

   const auto min_offset = m_dimensions.z - m_text_width - 2 * g_text_margin.x;
   if (event.global_mouse_position.x > m_dimensions.x + m_dimensions.w && m_text_offset > min_offset) {
      m_text_offset = std::max(m_text_offset - 1.0f, min_offset);
      this->update_text_position();
      this->update_selection_box();
   }

   const auto index = this->rune_index_from_offset(event.mouse_position.x);
   if (index == m_caret_position) {
      return;
   }

   this->disable_caret();

   if (index < m_caret_position) {
      m_selected_count += m_caret_position - index;
      m_caret_position = index;
   } else {
      m_selected_count = index - m_caret_position;
   }

   this->update_selection_box();
}

void TextInput::on_mouse_entered(const ui_core::Event& /*event*/)
{
   m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Edit);
   m_background_rect.set_color(m_context, TG_THEME_VAL(text_input.bg_hover));
}

void TextInput::on_mouse_left(const ui_core::Event& /*event*/)
{
   if (!m_is_active) {
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_background_rect.set_color(m_context, TG_THEME_VAL(text_input.bg_inactive));
   }
}

void TextInput::on_text_input(const ui_core::Event& /*event*/, const ui_core::Event::TextInput& text_input)
{
   if (!m_is_active)
      return;

   const auto rune = text_input.input_rune;
   if (font::Charset::European.contains(rune) && m_state.filter_func(rune)) {
      if (m_state.text.is_empty()) {
         this->update_text_position();
      }
      this->remove_selected();

      m_state.text.insert_rune_at(m_caret_position, rune);
      ++m_caret_position;
      this->recalculate_caret_offset();
      m_text_prim.set_content(m_context, m_state.text.view());
      event_OnTyping.publish(m_state.text.view());
   }
}

void TextInput::on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard)
{
   if (!m_is_active)
      return;

   switch (keyboard.key) {
   case desktop::Key::LeftArrow:
      if (m_caret_position != 0) {
         m_caret_position--;
      }
      this->recalculate_caret_offset();
      break;
   case desktop::Key::RightArrow:
      if (m_caret_position != m_state.text.size()) {
         m_caret_position++;
      }
      this->recalculate_caret_offset();
      break;
   case desktop::Key::Backspace: {
      if (!m_state.text.is_empty()) {
         if (m_selected_count != 0) {
            this->remove_selected();
         } else if (m_caret_position != 0) {
            m_caret_position--;
            m_state.text.remove_rune_at(m_caret_position);
            this->recalculate_caret_offset(true);
         }

         if (m_state.text.is_empty()) {
            m_text_prim.remove(m_context);
         } else {
            m_text_prim.set_content(m_context, m_state.text.view());
         }
         event_OnTyping.publish(m_state.text.view());
      }
      break;
   }
   case desktop::Key::Enter:
      [[fallthrough]];
   case desktop::Key::Escape: {
      this->on_deactivated(event);
      break;
   }
   default:
      break;
   }
}

void TextInput::on_activated(const ui_core::Event& /*event*/)
{
   this->enable_caret();
   if (m_is_active)
      return;

   m_is_active = true;

   using ET = ui_core::Event::Type;
   m_context.set_active_widget(this, m_dimensions, {ET::TextInput, ET::KeyPressed, ET::MouseMoved, ET::MouseReleased, ET::SelectAll});

   m_background_rect.set_color(m_context, TG_THEME_VAL(text_input.bg_active));
   m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Text | desktop::KeyboardInputMode::Direct);
}

void TextInput::on_deactivated(const ui_core::Event& /*event*/)
{
   m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
   m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Direct);
   m_background_rect.set_color(m_context, TG_THEME_VAL(text_input.bg_inactive));
   m_selection_rect.remove(m_context);
   m_selected_count = 0;
   m_is_active = false;
   this->disable_caret();
   event_OnTextChanged.publish(m_state.text.view());
}

void TextInput::on_select_all(const ui_core::Event& /*event*/)
{
   m_caret_position = 0;
   m_selected_count = static_cast<u32>(m_state.text.size());
   this->update_selection_box();
}

void TextInput::update_carret_state()
{
   m_is_carret_visible = !m_is_carret_visible;
   if (m_is_carret_visible) {
      m_caret_box.set_color(m_context, TG_THEME_VAL(foreground_color));
   } else {
      m_caret_box.set_color(m_context, {0, 0, 0, 0});
   }

   const auto duration = m_is_carret_visible ? 400ms : 200ms;
   m_timeout_handle = threading::Scheduler::the().register_timeout(duration, [this]() { this->update_carret_state(); });
}

void TextInput::set_content(const StringView content)
{
   m_state.text = content;
   m_text_offset = 0;
   m_caret_position = 0;
   m_selected_count = 0;
   m_text_prim.set_content(m_context, m_state.text.view());
   this->update_text_position();
}

const String& TextInput::content() const
{
   return m_state.text;
}

void TextInput::recalculate_caret_offset(const bool removal)
{
   float caret_offset = 0.0f;
   const auto rune_count = static_cast<u32>(m_state.text.rune_count());
   if (m_caret_position >= rune_count) {
      m_caret_position = rune_count;
   }

   const auto& props = m_state.manager->properties();

   if (m_caret_position != 0) {
      const auto substr = m_state.text.subview(0, static_cast<i32>(m_caret_position));

      auto& glyph_atlas = m_context.glyph_cache().find_glyph_atlas({props.base_typeface, props.button.font_size - 1});
      const auto measure = glyph_atlas.measure_text(substr);

      caret_offset = measure.width;
      if (removal && m_text_offset < 0 && m_caret_offset > caret_offset) {
         m_text_offset += m_caret_offset - caret_offset;
         if (m_text_offset > 0) {
            m_text_offset = 0;
         }
         this->update_text_position();
      } else if ((caret_offset + m_text_offset) > (m_dimensions.z - 2 * g_text_margin.x)) {
         m_text_offset = m_dimensions.z - 2 * g_text_margin.x - caret_offset;
         this->update_text_position();
      } else if ((caret_offset + m_text_offset) < 0) {
         m_text_offset = -caret_offset;
         this->update_text_position();
      }

      m_caret_offset = caret_offset;
   } else if (m_text_offset != 0) {
      m_text_offset = 0;
      this->update_text_position();
   }

   auto& glyph_atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), TG_THEME_VAL(button.font_size) - 1});
   const auto measure = glyph_atlas.measure_text(m_state.text.view());
   m_text_width = measure.width;

   m_caret_box.add(m_context,
                   {m_dimensions.x + g_text_margin.x + m_text_offset + caret_offset, m_dimensions.y + g_carret_margin, 1,
                    m_dimensions.w - 2 * g_carret_margin},
                   m_cropping_mask);
}

void TextInput::update_text_position()
{
   const Vector2 text_pos{m_text_xposition + m_text_offset, m_dimensions.y + m_text_size.y + g_text_margin.y};
   m_text_prim.add(m_context, text_pos, this->text_cropping_mask());
}

void TextInput::update_selection_box()
{
   if (m_selected_count == 0) {
      m_selection_rect.remove(m_context);
      return;
   }

   const auto offset_text = m_state.text.subview(0, static_cast<i32>(m_caret_position));
   const auto selected_text =
      m_state.text.subview(static_cast<i32>(m_caret_position), static_cast<i32>(m_caret_position + m_selected_count));

   auto& atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), TG_THEME_VAL(button.font_size) - 1});
   const auto offset_measure = atlas.measure_text(offset_text);
   const auto selected_measure = atlas.measure_text(selected_text);

   m_selection_rect.add(m_context,
                        {m_dimensions.x + g_text_margin.x + offset_measure.width + m_text_offset, m_dimensions.y + 0.5f * g_text_margin.y,
                         selected_measure.width, m_dimensions.w - g_text_margin.y},
                        this->text_cropping_mask());
}

Vector4 TextInput::text_cropping_mask() const
{
   const Vector4 text_crop{m_dimensions.x + g_text_margin.x, m_dimensions.y, m_dimensions.z - 2 * g_text_margin.x, m_dimensions.w};
   return min_area(text_crop, m_cropping_mask);
}

u32 TextInput::rune_index_from_offset(const float offset) const
{
   auto& atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), TG_THEME_VAL(button.font_size) - 1});
   return atlas.find_rune_index(m_state.text.view(), offset - g_text_margin.x - m_text_offset);
}

void TextInput::disable_caret()
{
   m_caret_box.set_color(m_context, {0, 0, 0, 0});
   if (m_timeout_handle.has_value()) {
      threading::Scheduler::the().cancel(*m_timeout_handle);
      m_timeout_handle.reset();
   }
}

void TextInput::enable_caret()
{
   if (!m_timeout_handle.has_value()) {
      this->update_carret_state();
   }
   this->recalculate_caret_offset();
}

void TextInput::remove_selected()
{
   if (m_selected_count == 0)
      return;
   m_state.text.remove_range(m_caret_position, m_caret_position + m_selected_count);
   m_selected_count = 0;
   this->update_selection_box();
   this->enable_caret();
}

}// namespace triglav::desktop_ui
