#include "LevelEditor.hpp"

#include "triglav/desktop_ui/CheckBox.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::editor {

using namespace name_literals;

LevelEditor::LevelEditor(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ProxyWidget(context, parent),
    m_state(state)
{
   auto& layout = this->create_content<ui_core::VerticalLayout>({});
   auto& toolbar = layout.create_child<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_brighter),
      .borderRadius = {0, 0, 0, 0},
      .borderColor = palette::NO_COLOR,
      .borderWidth = 0.0f,
   });

   auto& toolbar_layout = toolbar.create_content<ui_core::HorizontalLayout>({
      .padding = {8.0f, 6.0f, 8.0f, 6.0f},
      .separation = 4.0f,
      .gravity = ui_core::HorizontalAlignment::Left,
   });

   auto& select_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   select_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = Vector2{16.0f, 16.0f},
      .region = Vector4{192, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&select_btn);

   auto& move_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   move_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = Vector2{16.0f, 16.0f},
      .region = Vector4{0, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&move_btn);

   auto& rotate_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   rotate_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = Vector2{16.0f, 16.0f},
      .region = Vector4{64, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&rotate_btn);

   auto& scale_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   scale_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = Vector2{16.0f, 16.0f},
      .region = Vector4{128, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&scale_btn);

   auto& align_box = layout.create_child<ui_core::AlignmentBox>({
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Top,
   });
   align_box.create_content<ui_core::EmptySpace>({.size = {20, 20}});
}

}// namespace triglav::editor
