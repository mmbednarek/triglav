#include "LevelEditorSidePanel.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "SetTransformAction.hpp"
#include "src/RootWindow.hpp"
#include "triglav/Format.hpp"
#include "triglav/desktop_ui/Button.hpp"
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

   static constexpr auto num_only = [](const Rune r) -> bool { return std::isdigit(r) || r == '.' || r == '-'; };

   m_translateX = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = RED_OUTLINE,
   });
   m_translateY = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = GREEN_OUTLINE,
   });
   m_translateZ = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = BLUE_OUTLINE,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Rotate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });

   m_rotateX = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = RED_OUTLINE,
   });
   m_rotateY = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = GREEN_OUTLINE,
   });
   m_rotateZ = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = BLUE_OUTLINE,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Scale",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });

   m_scaleX = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = RED_OUTLINE,
   });
   m_scaleY = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = GREEN_OUTLINE,
   });
   m_scaleZ = &transform_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "0",
      .filter_func = num_only,
      .border_color = BLUE_OUTLINE,
   });

   auto& button = layout.create_child<desktop_ui::Button>({
      .manager = m_state.manager,
      .label = "Update",
   });
   TG_CONNECT_OPT(button, OnClick, apply_position);

   m_meshLabel = &layout.create_child<ui_core::TextBox>({
      .fontSize = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Mesh:",
      .color = TG_THEME_VAL(foreground_color),
      .horizontalAlignment = ui_core::HorizontalAlignment::Left,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });
}

void LevelEditorSidePanel::on_changed_selected_object(const renderer::SceneObject& object) const
{
   const auto x = format("{}", object.transform.translation.x);
   const auto y = format("{}", object.transform.translation.y);
   const auto z = format("{}", object.transform.translation.z);

   m_translateX->set_content(x.view());
   m_translateY->set_content(y.view());
   m_translateZ->set_content(z.view());

   const auto angles = glm::eulerAngles(object.transform.rotation);
   const auto yaw = format("{}", glm::degrees(angles.x));
   const auto pitch = format("{}", glm::degrees(angles.y));
   const auto roll = format("{}", glm::degrees(angles.z));

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

void LevelEditorSidePanel::apply_position(desktop::MouseButton /*mouse_button*/) const
{
   assert(m_state.editor);

   const float x = std::stof(m_translateX->content().to_std());
   const float y = std::stof(m_translateY->content().to_std());
   const float z = std::stof(m_translateZ->content().to_std());

   const float yaw = std::stof(m_rotateX->content().to_std());
   const float pitch = std::stof(m_rotateY->content().to_std());
   const float roll = std::stof(m_rotateZ->content().to_std());

   const float scale_x = std::stof(m_scaleX->content().to_std());
   const float scale_y = std::stof(m_scaleY->content().to_std());
   const float scale_z = std::stof(m_scaleZ->content().to_std());

   Transform3D transform{};
   transform.translation = Vector3{x, y, z};
   transform.rotation = glm::normalize(Quaternion{Vector3{glm::radians(yaw), glm::radians(pitch), glm::radians(roll)}});
   transform.scale = Vector3{scale_x, scale_y, scale_z};

   m_state.editor->history_manager().emplace_action<SetTransformAction>(*m_state.editor, m_state.editor->selected_object_id(),
                                                                        m_state.editor->selected_object()->transform, transform);

   m_state.editor->scene().set_transform(m_state.editor->selected_object_id(), transform);
   m_state.editor->viewport().update_view();
}

}// namespace triglav::editor
