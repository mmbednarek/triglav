#include "LevelEditorSidePanel.hpp"

#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::editor {

LevelEditorSidePanel::LevelEditorSidePanel(ui_core::Context& context, State state, IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
   auto& layout = this
                     ->create_content<ui_core::RectBox>({
                        .color = TG_THEME_VAL(background_color_brighter),
                        .borderRadius = {0, 0, 0, 0},
                        .borderColor = palette::NO_COLOR,
                        .borderWidth = 0.0f,
                     })
                     .create_content<ui_core::VerticalLayout>({
                        .padding = {10.0f, 10.0f, 10.0f, 10.0f},
                        .separation = 5.0f,
                     });

   layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size) + 1,
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Transform",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Top,
   });

   auto& translate_layout = layout.create_child<ui_core::HorizontalLayout>({
      .padding = {},
      .separation = 5.0f,
   });

   translate_layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Translate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });
   translate_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0.0",
      .width = 40.0f,
   });

   translate_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0.0",
      .width = 40.0f,
   });

   translate_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0.0",
      .width = 40.0f,
   });
}

}// namespace triglav::editor
