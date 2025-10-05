#include "TextInput.hpp"

#include "triglav/desktop/ISurface.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/threading/Scheduler.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/Primitives.hpp"
#include "triglav/ui_core/UICore.hpp"
#include "triglav/ui_core/Viewport.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <spdlog/spdlog.h>

namespace triglav::desktop_ui {

using namespace std::chrono_literals;
using namespace string_literals;

constexpr Vector2 g_textMargin{8, 15};
constexpr float g_carretMargin{12};

TextInput::TextInput(ui_core::Context& ctx, const TextInput::State state, ui_core::IWidget* /*parent*/) :
    m_context(ctx),
    m_state(state),
    m_rect(ctx,
           {
              .color = m_state.manager->properties().text_input_bg_inactive,
              .borderRadius = {5.0f, 5.0f, 5.0f, 5.0f},
              .borderColor = {0.1f, 0.1f, 0.1f, 1.f},
              .borderWidth = 1.0f,
           },
           this),
    m_caretPosition(static_cast<u32>(m_state.text.rune_count()))
{
   const auto& props = m_state.manager->properties();

   auto& glyphAtlas = m_context.glyph_cache().find_glyph_atlas({props.base_typeface, props.button_font_size});
   const auto measure = glyphAtlas.measure_text("0"_strv);

   m_textSize = {measure.width, measure.height};

   m_rect.create_content<ui_core::EmptySpace>({
      .size{m_state.width, 2 * g_textMargin.y + measure.height},
   });
}

Vector2 TextInput::desired_size(const Vector2 /*parentSize*/) const
{
   return {m_state.width, m_textSize.y + 30.0};
}

void TextInput::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_rect.add_to_viewport(dimensions, croppingMask);

   if (!do_regions_intersect(dimensions, croppingMask)) {
      if (m_caretBox != 0) {
         m_context.viewport().remove_rectangle(m_caretBox);
         m_caretBox = 0;
      }
      if (m_textPrim != 0) {
         m_context.viewport().remove_text(m_textPrim);
         m_textPrim = 0;
      }
      return;
   }

   const Vector4 caretDims{dimensions.x + g_textMargin.x, dimensions.y + g_carretMargin, 1, dimensions.w - 2 * g_carretMargin};

   if (m_caretBox != 0) {
      m_context.viewport().set_rectangle_dims(m_caretBox, caretDims, croppingMask);
   } else {
      m_caretBox = m_context.viewport().add_rectangle(ui_core::Rectangle{
         .rect = caretDims,
         .color = {0, 0, 0, 0},
         .borderRadius = {},
         .borderColor = {},
         .crop = croppingMask,
         .borderWidth = 0.0f,
      });
   }

   const auto& props = m_state.manager->properties();

   m_textXPosition = dimensions.x + g_textMargin.x;
   const Vector2 textPos{m_textXPosition - m_textOffset, dimensions.y + m_textSize.y + 15.0};
   Vector4 textCrop{dimensions.x + g_textMargin.x, dimensions.y, dimensions.x + dimensions.z - g_textMargin.x, dimensions.y + dimensions.w};
   textCrop = min_area(textCrop, croppingMask);

   if (m_textPrim != 0) {
      m_context.viewport().set_text_position(m_textPrim, textPos, textCrop);
   } else {
      m_textPrim = m_context.viewport().add_text(ui_core::Text{
         .content = m_state.text,
         .typefaceName = props.base_typeface,
         .fontSize = props.button_font_size,
         .position = textPos,
         .color = props.foreground_color,
         .crop = textCrop,
      });
   }

   m_dimensions = dimensions;
   m_croppingMask = croppingMask;

   this->recalculate_caret_offset();
}

void TextInput::remove_from_viewport()
{
   m_context.viewport().remove_rectangle(m_caretBox);
   m_context.viewport().remove_text(m_textPrim);
   m_rect.remove_from_viewport();

   m_caretBox = 0;
   m_textPrim = 0;
}

void TextInput::on_event(const ui_core::Event& event)
{
   if (m_textPrim == 0 || m_caretBox == 0) {
      return;
   }
   const auto& props = m_state.manager->properties();

   switch (event.eventType) {
   case ui_core::Event::Type::MousePressed: {
      m_isActive = true;
      m_rect.set_color(m_state.manager->properties().text_input_bg_active);
      m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Text | desktop::KeyboardInputMode::Direct);
      auto& glyphAtlas = m_context.glyph_cache().find_glyph_atlas({props.base_typeface, props.button_font_size});
      m_caretPosition = glyphAtlas.find_rune_index(m_state.text.view(), event.mousePosition.x - g_textMargin.x - m_textOffset);
      if (!m_timeoutHandle.has_value()) {
         this->update_carret_state();
      }
      this->recalculate_caret_offset();
      break;
   }
   case ui_core::Event::Type::MouseEntered:
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Edit);
      m_rect.set_color(m_state.manager->properties().text_input_bg_hover);
      break;
   case ui_core::Event::Type::MouseLeft:
      m_state.manager->surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_state.manager->surface().set_keyboard_input_mode(desktop::KeyboardInputMode::Direct);
      m_rect.set_color(m_state.manager->properties().text_input_bg_inactive);
      m_context.viewport().set_rectangle_color(m_caretBox, {0, 0, 0, 0});
      m_isActive = false;
      if (m_timeoutHandle.has_value()) {
         threading::Scheduler::the().cancel(*m_timeoutHandle);
         m_timeoutHandle.reset();
      }
      break;
   case ui_core::Event::Type::TextInput: {
      const auto rune = std::get<ui_core::Event::TextInput>(event.data).inputRune;
      if (font::Charset::European.contains(rune)) {
         m_state.text.insert_rune_at(m_caretPosition, rune);
         ++m_caretPosition;
         this->recalculate_caret_offset();
         m_context.viewport().set_text_content(m_textPrim, m_state.text.view());
      }
      break;
   }
   case ui_core::Event::Type::KeyPressed: {
      if (!m_isActive) {
         return;
      }
      const auto value = std::get<ui_core::Event::Keyboard>(event.data);
      switch (value.key) {
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
         if (!m_state.text.is_empty() && m_caretPosition != 0) {
            m_caretPosition--;
            m_state.text.remove_rune_at(m_caretPosition);
            this->recalculate_caret_offset(true);
            m_context.viewport().set_text_content(m_textPrim, m_state.text.view());
         }
         break;
      }
      default:
         break;
      }
   }
   default:
      break;
   }
}

void TextInput::update_carret_state()
{
   m_isCarretVisible = !m_isCarretVisible;
   if (m_isCarretVisible) {
      m_context.viewport().set_rectangle_color(m_caretBox, m_state.manager->properties().foreground_color);
   } else {
      m_context.viewport().set_rectangle_color(m_caretBox, {0, 0, 0, 0});
   }

   const auto duration = m_isCarretVisible ? 400ms : 200ms;
   m_timeoutHandle = threading::Scheduler::the().register_timeout(duration, [this]() { this->update_carret_state(); });
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

      auto& glyphAtlas = m_context.glyph_cache().find_glyph_atlas({props.base_typeface, props.button_font_size});
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

   m_context.viewport().set_rectangle_dims(m_caretBox,
                                           {m_dimensions.x + g_textMargin.x + m_textOffset + caretOffset, m_dimensions.y + g_carretMargin,
                                            1, m_dimensions.w - 2 * g_carretMargin},
                                           m_croppingMask);
}

void TextInput::update_text_position() const
{
   m_context.viewport().set_text_position(m_textPrim, {m_textXPosition + m_textOffset, m_dimensions.y + m_textSize.y + g_textMargin.y},
                                          m_dimensions);
}

}// namespace triglav::desktop_ui
