#include "LevelEditorSidePanel.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "SetTransformAction.hpp"
#include "src/RootWindow.hpp"
#include "triglav/Format.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/ui_core/widget/GridLayout.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <glm/gtx/euler_angles.hpp>

namespace triglav::editor {

constexpr Color RED_OUTLINE{1.0f, 0.28f, 0.28f, 1.0f};
constexpr Color GREEN_OUTLINE{0.35f, 1.0f, 0.35f, 1.0f};
constexpr Color BLUE_OUTLINE{0.17f, 0.5f, 1.0f, 1.0f};

TransposeInput::TransposeInput(ui_core::Context& context, State state, IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
   static constexpr auto num_only = [](const Rune r) -> bool { return std::isdigit(r) || r == '.' || r == '-'; };

   m_textInput = &this->create_content<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = m_state.border_color,
   });

   TG_CONNECT_OPT(*m_textInput, OnTextChanged, on_text_changed);
}

void TransposeInput::on_text_changed(const StringView text) const
{
   *m_state.destination = std::stof(String(text).to_std());
   m_state.side_panel->apply_transform();
}

void TransposeInput::set_content(const StringView text) const
{
   m_textInput->set_content(text);
}

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
                        .separation = 15.0f,
                     });

   layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size) + 1,
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Transform",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Top,
   });

   auto& transform_layout = layout.create_child<ui_core::GridLayout>({
      .column_ratios = {0.3f, 0.233f, 0.233f, 0.233f},
      .row_ratios = {0.333f, 0.333f, 0.333f},
      .horizontal_spacing = 5.0f,
      .vertical_spacing = 5.0f,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Translate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });

   m_translateX = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pendingTranslate.x,
   });
   m_translateY = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pendingTranslate.y,
   });
   m_translateZ = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pendingTranslate.z,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Rotate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });

   m_rotateX = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pendingRotation.x,
   });
   m_rotateY = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pendingRotation.y,
   });
   m_rotateZ = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pendingRotation.z,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Scale",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });

   m_scaleX = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pendingScale.x,
   });
   m_scaleY = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pendingScale.y,
   });
   m_scaleZ = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pendingScale.z,
   });

   m_meshLabel = &layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Mesh:",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });
}

void LevelEditorSidePanel::on_changed_selected_object(const renderer::SceneObject& object)
{
   m_pendingTranslate = object.transform.translation;
   m_pendingRotation = glm::eulerAngles(object.transform.rotation);
   m_pendingScale = object.transform.scale;

   const auto x = format("{}", object.transform.translation.x);
   const auto y = format("{}", object.transform.translation.y);
   const auto z = format("{}", object.transform.translation.z);

   m_translateX->set_content(x.view());
   m_translateY->set_content(y.view());
   m_translateZ->set_content(z.view());

   const auto yaw = format("{}", glm::degrees(m_pendingRotation.x));
   const auto pitch = format("{}", glm::degrees(m_pendingRotation.y));
   const auto roll = format("{}", glm::degrees(m_pendingRotation.z));

   m_rotateX->set_content(yaw.view());
   m_rotateY->set_content(pitch.view());
   m_rotateZ->set_content(roll.view());

   const auto scale_x = format("{}", object.transform.scale.x);
   const auto scale_y = format("{}", object.transform.scale.y);
   const auto scale_z = format("{}", object.transform.scale.z);

   m_scaleX->set_content(scale_x.view());
   m_scaleY->set_content(scale_y.view());
   m_scaleZ->set_content(scale_z.view());

   std::string mesh_path = m_state.editor->root_window().resource_manager().lookup_name(object.model).value_or("");
   auto content = triglav::format("Mesh: {}", mesh_path);
   m_meshLabel->set_content(content.view());
}

void LevelEditorSidePanel::apply_transform() const
{
   assert(m_state.editor);

   if (m_state.editor->selected_object() == nullptr)
      return;

   Transform3D transform{};
   transform.translation = m_pendingTranslate;
   transform.rotation = Quaternion{m_pendingRotation};
   transform.scale = m_pendingScale;

   m_state.editor->history_manager().emplace_action<SetTransformAction>(*m_state.editor, m_state.editor->selected_object_id(),
                                                                        m_state.editor->selected_object()->transform, transform);

   m_state.editor->scene().set_transform(m_state.editor->selected_object_id(), transform);
   m_state.editor->viewport().update_view();
}

}// namespace triglav::editor
