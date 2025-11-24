#include "SceneView.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/Padding.hpp"

namespace triglav::editor {

SceneView::SceneView(ui_core::Context& context, State state, IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(std::move(state)),
    TG_CONNECT(m_state.editor->scene(), OnObjectAddedToScene, on_object_added_to_scene)
{
   auto& vert_layout = this
                          ->create_content<ui_core::RectBox>({
                             .color = TG_THEME_VAL(background_color_brighter),
                             .border_radius = {0, 0, 0, 0},
                             .border_color = palette::NO_COLOR,
                             .border_width = 0.0f,
                          })
                          .create_content<ui_core::Padding>({4, 4, 4, 4})
                          .create_content<ui_core::VerticalLayout>({
                             .padding = {0, 0, 0, 0},
                             .separation = 0.0f,
                          });

   auto& buttons = vert_layout.create_child<ui_core::HorizontalLayout>({
      .padding = {5.0f, 5.0f, 5.0f, 5.0f},
      .separation = 10.0f,
      .gravity = ui_core::HorizontalAlignment::Left,
   });

   auto& add_button = buttons.create_child<desktop_ui::Button>({
      .manager = m_state.manager,
   });
   add_button.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .max_size = Vector2{20, 20},
      .region = Vector4{5 * 64, 0, 64, 64},
   });
   TG_CONNECT_NAMED_OPT(add_button, OnClick, Add, on_clicked_add);

   auto& add_dir_button = buttons.create_child<desktop_ui::Button>({
      .manager = m_state.manager,
   });
   add_dir_button.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .max_size = Vector2{20, 20},
      .region = Vector4{5 * 64, 64, 64, 64},
   });
   TG_CONNECT_NAMED_OPT(add_dir_button, OnClick, AddDirectory, on_clicked_add_directory);

   auto& delete_button = buttons.create_child<desktop_ui::Button>({
      .manager = m_state.manager,
   });
   delete_button.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .max_size = Vector2{20, 20},
      .region = Vector4{5 * 64, 2 * 64, 64, 64},
   });
   TG_CONNECT_NAMED_OPT(delete_button, OnClick, Delete, on_clicked_delete);

   m_tree_view = &vert_layout
                     .create_child<ui_core::RectBox>({
                        .color = TG_THEME_VAL(background_color_darker),
                        .border_radius = {4, 4, 4, 4},
                        .border_color = palette::NO_COLOR,
                        .border_width = 0.0f,
                     })
                     .create_content<ui_core::AlignmentBox>({
                        .horizontal_alignment = std::nullopt,
                        .vertical_alignment = ui_core::VerticalAlignment::Top,
                     })
                     .create_content<desktop_ui::TreeView>({
                        .manager = m_state.manager,
                        .controller = &m_tree_controller,
                        .extended_items = {},
                     });
   TG_CONNECT_OPT(*m_tree_view, OnSelected, on_selected_object);
}

void SceneView::on_selected_object(const desktop_ui::TreeItemId item_id)
{
   const auto object_id = m_item_id_to_object_id[item_id];
   m_state.editor->set_selected_object(object_id);
   m_state.editor->viewport().update_view();
}

void SceneView::on_clicked_add()
{
   log_message(LogLevel::Info, StringView{"TESTING"}, "Clicked Add");
}

void SceneView::on_clicked_add_directory()
{
   log_message(LogLevel::Info, StringView{"TESTING"}, "Clicked Add Directory");
}

void SceneView::on_clicked_delete()
{
   m_state.editor->remove_selected_item();
   log_message(LogLevel::Info, StringView{"SceneView"}, "Deleted Item");
}

void SceneView::on_object_added_to_scene(const renderer::ObjectID object_id, const renderer::SceneObject& object)
{
   const auto id = m_tree_controller.add_item(0, {
                                                    .icon_name = "texture/ui_atlas.tex"_rc,
                                                    .icon_region = {4 * 64, 3 * 64, 64, 64},
                                                    .label = object.name,
                                                    .has_children = false,
                                                 });
   m_item_id_to_object_id[id] = object_id;
   m_object_id_to_item_id[object_id] = id;
}

void SceneView::on_object_is_removed(const renderer::ObjectID object_id) const
{
   m_tree_view->remove_item(m_object_id_to_item_id.at(object_id));
}

void SceneView::update_selected_item() const
{
   m_tree_view->set_selected_item(m_object_id_to_item_id.at(m_state.editor->selected_object_id()));
}

}// namespace triglav::editor