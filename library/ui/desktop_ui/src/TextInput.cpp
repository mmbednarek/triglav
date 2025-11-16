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

constexpr Vector2 g_textMargin{8, 10};
constexpr float g_carretMargin{6};

TextInput::TextInput(ui_core::Context& ctx, const TextInput::State state, ui_core::IWidget* /*parent*/) :
    m_context(ctx),
    m_state(state),
    m_backgroundRect{
       .color = TG_THEME_VAL(text_input.bg_inactive),
       .border_radius = {8, 8, 8, 8},
       .border_color = m_state.border_color,
       .border_width = 1.0f,
    },
    m_selectionRect{
       .color = {0.5, 0.5, 0.5, 1.0},
       .border_radius = {0, 0, 0, 0},
       .border_color = palette::NO_COLOR,
       .border_width = 0.0f,
    },
    m_textPrim{
       .content = m_state.text,
       .typeface_name = TG_THEME_VAL(base_typeface),
       .font_size = TG_THEME_VAL(button.font_size) - 1,
       .color = TG_THEME_VAL(foreground_color),
    },
    m_caretBox{
       .color = {0, 0, 0, 0},
       .border_radius = {},
       .border_color = {},
       .border_width = 0.0f,
    },
    m_caretPosition(static_cast<u32>(m_state.text.rune_count()))
{
   const auto& props = m_state.manager->properties();

   auto& glyphAtlas = m_context.glyph_cache().find_glyph_atlas({props.base_typeface, props.button.font_size});
   const auto measure = glyphAtlas.measure_text("0"_strv);

   m_textSize = {measure.width, measure.height};
}

Vector2 TextInput::desired_size(const Vector2 parentSize) const
{
   return {parentSize.x, m_textSize.y + 2 * g_textMargin.y};
}

void TextInput::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   if (!do_regions_intersect(dimensions, croppingMask)) {
      m_backgroundRect.remove(m_context);
      m_caretBox.remove(m_context);
      m_textPrim.remove(m_context);
      return;
   }

   m_textOffset = 0.0f;

   m_backgroundRect.add(m_context, dimensions, croppingMask);

   this->update_selection_box();

   const Vector4 caret_dims{dimensions.x + g_textMargin.x, dimensions.y + g_carretMargin, 1, dimensions.w - 2 * g_carretMargin};
   m_caretBox.add(m_context, caret_dims, croppingMask);

   m_textXPosition = dimensions.x + g_textMargin.x;
   m_dimensions = dimensions;
   m_croppingMask = croppingMask;
   this->update_text_position();
   this->recalculate_caret_offset();
}

void TextInput::remove_from_viewport()
{
   m_backgroundRect.remove(m_context);
   m_selectionRect.remove(m_context);
   m_caretBox.remove(m_context);
   m_textPrim.remove(m_context);
}

void TextInput::on_event(const ui_core::Event& event)
{
   ui_core::visit_event<void>(*this, event);
}

void TextInput::on_mouse_pressed(const ui_core::Event& event, const ui_core::Event::Mouse& /*mouse*/)
{
   m_caretPosition = this->rune_index_from_offset(event.mousePosition.x);
   this->on_activated(event);
   m_selectedCount = 0;
   m_isSelecting = true;
   this->update_selection_box();
}

void TextInput::on_mouse_released(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/)
{
   m_isSelecting = false;
}

void TextInput::on_mouse_moved(const ui_core::Event& event)
{
   if (!m_isSelecting || !event.isForwardedToActive)
      return;

   if (event.globalMousePosition.x < m_dimensions.x && m_textOffset < 0.0f) {
      m_textOffset += 0.2f;
      this->update_text_position();
      this->update_selection_box();
   } else if (event.globalMousePosition.x > m_dimensions.x + m_dimensions.w) {
      m_textOffset -= 0.2f;
      this->update_text_position();
      this->update_selection_box();
   }

   const auto index = this->rune_index_from_offset(event.mousePosition.x);
   if (index == m_caretPosition) {
      return;
   }

   this->disable_caret();

   if (index < m_caretPosition) {
      m_selectedCount += m_caretPosition - index;
      m_caretPosition = index;
   } else {
      m_selectedCount = index - m_caretPosition;
   }

   this->update_selection_box();
}

void TextInput::on_mouse_entered(const ui_core::Event& /*event*/)
{
   m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Edit);
   m_backgroundRect.set_color(m_context, TG_THEME_VAL(text_input.bg_hover));
}

void TextInput::on_mouse_left(const ui_core::Event& /*event*/)
{
   if (!m_isActive) {
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_backgroundRect.set_color(m_context, TG_THEME_VAL(text_input.bg_inactive));
   }
}

void TextInput::on_text_input(const ui_core::Event& event, const ui_core::Event::TextInput& text_input)
{
   if (!m_isActive || !event.isForwardedToActive)
      return;

   const auto rune = text_input.inputRune;
   if (font::Charset::European.contains(rune) && m_state.filter_func(rune)) {
      if (m_state.text.is_empty()) {
         this->update_text_position();
      }
      this->remove_selected();

      m_state.text.insert_rune_at(m_caretPosition, rune);
      ++m_caretPosition;
      this->recalculate_caret_offset();
      m_textPrim.set_content(m_context, m_state.text.view());
   }
}

void TextInput::on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard)
{
   if (!m_isActive || !event.isForwardedToActive)
      return;

   switch (keyboard.key) {
   case desktop::Key::LeftArrow:
      if (m_caretPosition != 0) {
         m_caretPosition--;
      }
      this->recalculate_caret_offset();
      break;
   case desktop::Key::RightArrow:
      if (m_caretPosition != m_state.text.size()) {
         m_caretPosition++;
      }
      this->recalculate_caret_offset();
      break;
   case desktop::Key::Backspace: {
      if (!m_state.text.is_empty()) {
         if (m_selectedCount != 0) {
            this->remove_selected();
         } else if (m_caretPosition != 0) {
            m_caretPosition--;
            m_state.text.remove_rune_at(m_caretPosition);
            this->recalculate_caret_offset(true);
         }

         if (m_state.text.is_empty()) {
            m_textPrim.remove(m_context);
         } else {
            m_textPrim.set_content(m_context, m_state.text.view());
         }
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
   if (m_isActive)
      return;

   m_isActive = true;
   m_context.set_active_widget(this, m_dimensions);

   m_backgroundRect.set_color(m_context, TG_THEME_VAL(text_input.bg_active));
   m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Text | desktop::KeyboardInputMode::Direct);
}

void TextInput::on_deactivated(const ui_core::Event& /*event*/)
{
   m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
   m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Direct);
   m_backgroundRect.set_color(m_context, TG_THEME_VAL(text_input.bg_inactive));
   m_isActive = false;
   this->disable_caret();
   event_OnTextChanged.publish(m_state.text.view());
}

void TextInput::on_select_all(const ui_core::Event& /*event*/)
{
   m_caretPosition = 0;
   m_selectedCount = m_state.text.size();
   this->update_selection_box();
}

void TextInput::update_carret_state()
{
   m_isCarretVisible = !m_isCarretVisible;
   if (m_isCarretVisible) {
      m_caretBox.set_color(m_context, TG_THEME_VAL(foreground_color));
   } else {
      m_caretBox.set_color(m_context, {0, 0, 0, 0});
   }

   const auto duration = m_isCarretVisible ? 400ms : 200ms;
   m_timeoutHandle = threading::Scheduler::the().register_timeout(duration, [this]() { this->update_carret_state(); });
}

void TextInput::set_content(const StringView content)
{
   m_state.text = content;
   m_textOffset = 0;
   m_caretPosition = 0;
   m_selectedCount = 0;
   m_textPrim.set_content(m_context, m_state.text.view());
   this->update_text_position();
}

const String& TextInput::content() const
{
   return m_state.text;
}

void TextInput::recalculate_caret_offset(const bool removal)
{
   float caretOffset = 0.0f;
   const auto runeCount = static_cast<u32>(m_state.text.rune_count());
   if (m_caretPosition >= runeCount) {
      m_caretPosition = runeCount;
   }

   const auto& props = m_state.manager->properties();

   if (m_caretPosition != 0) {
      const auto substr = m_state.text.subview(0, static_cast<i32>(m_caretPosition));

      auto& glyphAtlas = m_context.glyph_cache().find_glyph_atlas({props.base_typeface, props.button.font_size - 1});
      const auto measure = glyphAtlas.measure_text(substr);

      caretOffset = measure.width;
      if (removal && m_textOffset < 0 && m_caretOffset > caretOffset) {
         m_textOffset += m_caretOffset - caretOffset;
         if (m_textOffset > 0) {
            m_textOffset = 0;
         }
         this->update_text_position();
      } else if ((caretOffset + m_textOffset) > (m_dimensions.z - 2 * g_textMargin.x)) {
         m_textOffset = m_dimensions.z - 2 * g_textMargin.x - caretOffset;
         this->update_text_position();
      } else if ((caretOffset + m_textOffset) < 0) {
         m_textOffset = -caretOffset;
         this->update_text_position();
      }

      m_caretOffset = caretOffset;
   } else if (m_textOffset != 0) {
      m_textOffset = 0;
      this->update_text_position();
   }

   m_caretBox.add(m_context,
                  {m_dimensions.x + g_textMargin.x + m_textOffset + caretOffset, m_dimensions.y + g_carretMargin, 1,
                   m_dimensions.w - 2 * g_carretMargin},
                  m_croppingMask);
}

void TextInput::update_text_position()
{
   const Vector2 textPos{m_textXPosition + m_textOffset, m_dimensions.y + m_textSize.y + g_textMargin.y};
   m_textPrim.add(m_context, textPos, this->text_cropping_mask());
}

void TextInput::update_selection_box()
{
   if (m_selectedCount == 0) {
      m_selectionRect.remove(m_context);
      return;
   }

   const auto offset_text = m_state.text.subview(0, static_cast<i32>(m_caretPosition));
   const auto selected_text = m_state.text.subview(static_cast<i32>(m_caretPosition), static_cast<i32>(m_caretPosition + m_selectedCount));

   auto& atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), TG_THEME_VAL(button.font_size) - 1});
   const auto offset_measure = atlas.measure_text(offset_text);
   const auto selected_measure = atlas.measure_text(selected_text);

   m_selectionRect.add(m_context,
                       {m_dimensions.x + g_textMargin.x + offset_measure.width + m_textOffset, m_dimensions.y + 0.5f * g_textMargin.y,
                        selected_measure.width, m_dimensions.w - g_textMargin.y},
                       this->text_cropping_mask());
}

Vector4 TextInput::text_cropping_mask() const
{
   const Vector4 textCrop{m_dimensions.x + g_textMargin.x, m_dimensions.y, m_dimensions.z - 2 * g_textMargin.x, m_dimensions.w};
   return min_area(textCrop, m_croppingMask);
}

u32 TextInput::rune_index_from_offset(const float offset) const
{
   auto& atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), TG_THEME_VAL(button.font_size) - 1});
   return atlas.find_rune_index(m_state.text.view(), offset - g_textMargin.x - m_textOffset);
}

void TextInput::disable_caret()
{
   m_caretBox.set_color(m_context, {0, 0, 0, 0});
   if (m_timeoutHandle.has_value()) {
      threading::Scheduler::the().cancel(*m_timeoutHandle);
      m_timeoutHandle.reset();
   }
}

void TextInput::enable_caret()
{
   if (!m_timeoutHandle.has_value()) {
      this->update_carret_state();
   }
   this->recalculate_caret_offset();
}

void TextInput::remove_selected()
{
   if (m_selectedCount == 0)
      return;
   m_state.text.remove_range(m_caretPosition, m_caretPosition + m_selectedCount);
   m_selectedCount = 0;
   this->update_selection_box();
   this->enable_caret();
}

}// namespace triglav::desktop_ui
