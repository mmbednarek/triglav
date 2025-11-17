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

   m_text_input = &this->create_content<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = m_state.border_color,
   });

   TG_CONNECT_OPT(*m_text_input, OnTextChanged, on_text_changed);
}

void TransposeInput::on_text_changed(const StringView text) const
{
   *m_state.destination = std::stof(String(text).to_std());
   m_state.side_panel->apply_transform();
}

void TransposeInput::set_content(const StringView text) const
{
   m_text_input->set_content(text);
}

LevelEditorSidePanel::LevelEditorSidePanel(ui_core::Context& context, State state, IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
   auto& layout = this
                     ->create_content<ui_core::RectBox>({
                        .color = TG_THEME_VAL(background_color_brighter),
                        .border_radius = {0, 0, 0, 0},
                        .border_color = palette::NO_COLOR,
                        .border_width = 0.0f,
                     })
                     .create_content<ui_core::VerticalLayout>({
                        .padding = {10.0f, 10.0f, 10.0f, 10.0f},
                        .separation = 15.0f,
                     });

   layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size) + 1,
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Transform",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Top,
   });

   auto& transform_layout = layout.create_child<ui_core::GridLayout>({
      .column_ratios = {0.3f, 0.233f, 0.233f, 0.233f},
      .row_ratios = {0.333f, 0.333f, 0.333f},
      .horizontal_spacing = 5.0f,
      .vertical_spacing = 5.0f,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Translate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });

   m_translate_x = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pending_translate.x,
   });
   m_translate_y = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pending_translate.y,
   });
   m_translate_z = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pending_translate.z,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Rotate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });

   m_rotate_x = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pending_rotation.x,
   });
   m_rotate_y = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pending_rotation.y,
   });
   m_rotate_z = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pending_rotation.z,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Scale",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });

   m_scale_x = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pending_scale.x,
   });
   m_scale_y = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pending_scale.y,
   });
   m_scale_z = &transform_layout.create_child<TransposeInput>({
      .manager = m_state.manager,
      .side_panel = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pending_scale.z,
   });

   m_mesh_label = &layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Mesh:",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });
}

void LevelEditorSidePanel::on_changed_selected_object(const renderer::SceneObject& object)
{
   m_pending_translate = object.transform.translation;
   m_pending_rotation = glm::eulerAngles(object.transform.rotation);
   m_pending_scale = object.transform.scale;

   const auto x = format("{}", object.transform.translation.x);
   const auto y = format("{}", object.transform.translation.y);
   const auto z = format("{}", object.transform.translation.z);

   m_translate_x->set_content(x.view());
   m_translate_y->set_content(y.view());
   m_translate_z->set_content(z.view());

   const auto yaw = format("{}", glm::degrees(m_pending_rotation.x));
   const auto pitch = format("{}", glm::degrees(m_pending_rotation.y));
   const auto roll = format("{}", glm::degrees(m_pending_rotation.z));

   m_rotate_x->set_content(yaw.view());
   m_rotate_y->set_content(pitch.view());
   m_rotate_z->set_content(roll.view());

   const auto scale_x = format("{}", object.transform.scale.x);
   const auto scale_y = format("{}", object.transform.scale.y);
   const auto scale_z = format("{}", object.transform.scale.z);

   m_scale_x->set_content(scale_x.view());
   m_scale_y->set_content(scale_y.view());
   m_scale_z->set_content(scale_z.view());

   std::string mesh_path = m_state.editor->root_window().resource_manager().lookup_name(object.model).value_or("");
   auto content = triglav::format("Mesh: {}", mesh_path);
   m_mesh_label->set_content(content.view());
}

void LevelEditorSidePanel::apply_transform() const
{
   assert(m_state.editor);

   if (m_state.editor->selected_object() == nullptr)
      return;

   Transform3D transform{};
   transform.translation = m_pending_translate;
   transform.rotation = Quaternion{m_pending_rotation};
   transform.scale = m_pending_scale;

   m_state.editor->history_manager().emplace_action<SetTransformAction>(*m_state.editor, m_state.editor->selected_object_id(),
                                                                        m_state.editor->selected_object()->transform, transform);

   m_state.editor->scene().set_transform(m_state.editor->selected_object_id(), transform);
   m_state.editor->viewport().update_view();
}

}// namespace triglav::editor
