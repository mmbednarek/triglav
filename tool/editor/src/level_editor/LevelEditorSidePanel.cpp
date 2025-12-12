#include "LevelEditorSidePanel.hpp"

#include "../RootWindow.hpp"
#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "SceneView.hpp"
#include "SetTransformAction.hpp"

#include "triglav/Format.hpp"
#include "triglav/desktop_ui/Splitter.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/GridLayout.hpp"
#include "triglav/ui_core/widget/HideableWidget.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <glm/gtx/euler_angles.hpp>

namespace triglav::editor {

using namespace name_literals;

constexpr Color RED_OUTLINE{1.0f, 0.28f, 0.28f, 1.0f};
constexpr Color GREEN_OUTLINE{0.35f, 1.0f, 0.35f, 1.0f};
constexpr Color BLUE_OUTLINE{0.17f, 0.5f, 1.0f, 1.0f};

class PanelHeader final : public ui_core::ProxyWidget
{
 public:
   struct State
   {
      desktop_ui::DesktopUIManager* manager{};
      String label;
   };

   PanelHeader(ui_core::Context& context, State state, IWidget* parent) :
       ui_core::ProxyWidget(context, parent),
       m_state(std::move(state))
   {
      auto& layout = this->create_content<ui_core::VerticalLayout>({
         .padding = {},
         .separation = 10.0f,
      });

      layout.create_child<ui_core::TextBox>({
         .font_size = TG_THEME_VAL(base_font_size),
         .typeface = TG_THEME_VAL(base_typeface),
         .content = m_state.label,
         .color = TG_THEME_VAL(accent_color),
         .horizontal_alignment = ui_core::HorizontalAlignment::Left,
         .vertical_alignment = ui_core::VerticalAlignment::Top,
      });

      layout
         .create_child<ui_core::RectBox>({
            .color = TG_THEME_VAL(accent_color),
            .border_radius = {0, 0, 0, 0},
            .border_color = palette::NO_COLOR,
            .border_width = 0.0f,
         })
         .create_content<ui_core::EmptySpace>({
            .size = {10.0f, 1.0f},
         });
      layout.create_child<ui_core::EmptySpace>({
         .size = {10.0f, 5.0f},
      });
   }

 private:
   State m_state;
};

class TransformInput;

class TransformWidget final : public ui_core::ProxyWidget
{
 public:
   struct State
   {
      desktop_ui::DesktopUIManager* manager{};
      LevelEditor* editor{};
   };

   TransformWidget(ui_core::Context& context, State state, IWidget* parent);

   void on_changed_selected_object(const renderer::SceneObject& object);
   void apply_transform();

 private:
   State m_state;

   TransformInput* m_translate_x;
   TransformInput* m_translate_y;
   TransformInput* m_translate_z;

   TransformInput* m_rotate_x;
   TransformInput* m_rotate_y;
   TransformInput* m_rotate_z;

   TransformInput* m_scale_x;
   TransformInput* m_scale_y;
   TransformInput* m_scale_z;

   Vector3 m_pending_translate{};
   Vector3 m_pending_rotation{};
   Vector3 m_pending_scale{};
};

class TransformInput final : public ui_core::ProxyWidget
{
 public:
   using Self = TransformInput;

   struct State
   {
      desktop_ui::DesktopUIManager* manager{};
      TransformWidget* widget;
      Color border_color{};
      float* destination;
   };

   TransformInput(ui_core::Context& context, State state, IWidget* parent) :
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

   void on_text_changed(const StringView text) const
   {
      *m_state.destination = std::stof(String(text).to_std());
      m_state.widget->apply_transform();
   }

   void set_content(const StringView text) const
   {
      m_text_input->set_content(text);
   }

 private:
   State m_state;
   desktop_ui::TextInput* m_text_input{};

   TG_OPT_SINK(desktop_ui::TextInput, OnTextChanged);
};

TransformWidget::TransformWidget(ui_core::Context& context, State state, IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
   auto& transform_layout = this->create_content<ui_core::GridLayout>({
      .column_ratios = {0.3f, 0.233f, 0.233f, 0.233f},
      .row_ratios = {0.333f, 0.333f, 0.333f},
      .horizontal_spacing = 5.0f,
      .vertical_spacing = 5.0f,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size) - 1,
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Translate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });

   m_translate_x = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pending_translate.x,
   });
   m_translate_y = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pending_translate.y,
   });
   m_translate_z = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pending_translate.z,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size) - 1,
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Rotate",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });

   m_rotate_x = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pending_rotation.x,
   });
   m_rotate_y = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pending_rotation.y,
   });
   m_rotate_z = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pending_rotation.z,
   });

   transform_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size) - 1,
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Scale",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });

   m_scale_x = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = RED_OUTLINE,
      .destination = &m_pending_scale.x,
   });
   m_scale_y = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = GREEN_OUTLINE,
      .destination = &m_pending_scale.y,
   });
   m_scale_z = &transform_layout.create_child<TransformInput>({
      .manager = m_state.manager,
      .widget = this,
      .border_color = BLUE_OUTLINE,
      .destination = &m_pending_scale.z,
   });
}

void TransformWidget::on_changed_selected_object(const renderer::SceneObject& object)
{
   m_pending_translate = object.transform.translation;
   m_pending_rotation = glm::degrees(glm::eulerAngles(object.transform.rotation));
   m_pending_scale = object.transform.scale;

   const auto x = format("{}", object.transform.translation.x);
   const auto y = format("{}", object.transform.translation.y);
   const auto z = format("{}", object.transform.translation.z);

   m_translate_x->set_content(x.view());
   m_translate_y->set_content(y.view());
   m_translate_z->set_content(z.view());

   const auto yaw = format("{}", m_pending_rotation.x);
   const auto pitch = format("{}", m_pending_rotation.y);
   const auto roll = format("{}", m_pending_rotation.z);

   m_rotate_x->set_content(yaw.view());
   m_rotate_y->set_content(pitch.view());
   m_rotate_z->set_content(roll.view());

   const auto scale_x = format("{}", object.transform.scale.x);
   const auto scale_y = format("{}", object.transform.scale.y);
   const auto scale_z = format("{}", object.transform.scale.z);

   m_scale_x->set_content(scale_x.view());
   m_scale_y->set_content(scale_y.view());
   m_scale_z->set_content(scale_z.view());
}

void TransformWidget::apply_transform()
{
   assert(m_state.editor);

   if (m_state.editor->selected_object() == nullptr)
      return;

   Transform3D transform{};
   transform.translation = m_pending_translate;
   transform.rotation = Quaternion{glm::radians(m_pending_rotation)};
   transform.scale = m_pending_scale;

   m_state.editor->history_manager().emplace_action<SetTransformAction>(*m_state.editor, m_state.editor->selected_object_id(),
                                                                        m_state.editor->selected_object()->transform, transform);

   m_state.editor->scene().set_transform(m_state.editor->selected_object_id(), transform);
   m_state.editor->viewport().update_view();
}

LevelEditorSidePanel::LevelEditorSidePanel(ui_core::Context& context, State state, IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state)
{
   auto& split = this->create_content<desktop_ui::Splitter>({
      .manager = m_state.manager,
      .offset = 300,
      .axis = ui_core::Axis::Vertical,
      .offset_type = desktop_ui::SplitterOffsetType::Preceeding,
   });

   m_scene_view = &split.create_preceding<SceneView>({
      .manager = m_state.manager,
      .editor = m_state.editor,
   });

   m_object_info = &split
                       .create_following<ui_core::RectBox>({
                          .color = TG_THEME_VAL(background_color_brighter),
                          .border_radius = {0, 0, 0, 0},
                          .border_color = palette::NO_COLOR,
                          .border_width = 0.0f,
                       })
                       .create_content<ui_core::HideableWidget>({
                          .is_hidden = true,
                       });

   auto& layout = m_object_info->create_content<ui_core::VerticalLayout>({
      .padding = {10.0f, 10.0f, 10.0f, 10.0f},
      .separation = 7.0f,
   });

   layout.create_child<PanelHeader>({.manager = m_state.manager, .label = "Transform"});

   m_transform_widget = &layout.create_child<TransformWidget>({
      .manager = m_state.manager,
      .editor = m_state.editor,
   });

   layout.create_child<ui_core::EmptySpace>({
      .size = {10.0f, 5.0f},
   });

   layout.create_child<PanelHeader>({.manager = m_state.manager, .label = "Properties"});

   auto& prop_layout = layout.create_child<ui_core::GridLayout>({
      .column_ratios = {0.3f, 0.7f},
      .row_ratios = {0.5f, 0.5f},
      .horizontal_spacing = 5.0f,
      .vertical_spacing = 5.0f,
   });

   prop_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Name",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });
   m_name_input = &prop_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "",
      .border_color = {0.3f, 0.3f, 0.3f, 1.0f},
   });
   TG_CONNECT_NAMED_OPT(*m_name_input, OnTextChanged, NameChange, on_changed_name);

   prop_layout.create_child<ui_core::TextBox>({
      .font_size = TG_THEME_VAL(base_font_size),
      .typeface = TG_THEME_VAL(base_typeface),
      .content = "Mesh",
      .color = TG_THEME_VAL(foreground_color),
      .horizontal_alignment = ui_core::HorizontalAlignment::Left,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });
   m_mesh_input = &prop_layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "",
      .border_color = {0.3f, 0.3f, 0.3f, 1.0f},
   });
   TG_CONNECT_NAMED_OPT(*m_mesh_input, OnTextChanged, MeshChange, on_changed_mesh);
}

void LevelEditorSidePanel::on_unselected() const
{
   if (!m_object_info->state().is_hidden) {
      m_object_info->set_is_hidden(true);
   }
}

void LevelEditorSidePanel::on_changed_selected_object(const renderer::SceneObject& object) const
{
   m_object_info->set_is_hidden(false);
   m_transform_widget->on_changed_selected_object(object);

   m_name_input->set_content(object.name.view());

   const std::string mesh_path = m_state.editor->root_window().resource_manager().lookup_name(object.model).value_or("");
   m_mesh_input->set_content(StringView{mesh_path});

   m_scene_view->update_selected_item();
}

void LevelEditorSidePanel::on_object_is_removed(const renderer::ObjectID object_id) const
{
   m_scene_view->on_object_is_removed(object_id);
}

void LevelEditorSidePanel::on_changed_name(const StringView name) const
{
   m_state.editor->set_selected_name(name);
}

void LevelEditorSidePanel::on_changed_mesh(StringView /*mesh*/)
{
   // TODO: Implement
}

}// namespace triglav::editor
