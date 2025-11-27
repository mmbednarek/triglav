#include "SceneView.hpp"

#include "../ResourceSelector.hpp"
#include "LevelEditor.hpp"
#include "LevelViewport.hpp"

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/Padding.hpp"

namespace triglav::editor {

using namespace string_literals;

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

   auto& add_trigger = buttons.create_child<ResourceSelectTrigger>({
      .manager = m_state.manager,
      .suffix = ".mesh",
   });
   TG_CONNECT_NAMED_OPT(add_trigger, OnSelected, OnResourceSelected, on_resource_selected);

   auto& add_button = add_trigger.create_content<desktop_ui::Button>({
      .manager = m_state.manager,
   });
   add_button.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .max_size = Vector2{20, 20},
      .region = Vector4{5 * 64, 0, 64, 64},
   });

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

void SceneView::on_clicked_add_directory()
{
   log_info("Clicked add directory!");
}

void SceneView::on_clicked_delete()
{
   m_state.editor->remove_selected_item();
   log_info("Deleted item!");
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

void SceneView::on_resource_selected(const String resource) const
{
   log_info("Selected resource: {}", resource.to_std());

   const auto& camera = m_state.editor->scene().camera();

   Transform3D transform{};
   transform.translation = camera.position() + 10.0f * camera.forward_vector();
   transform.rotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
   transform.scale = {1, 1, 1};

   const auto object_id = m_state.editor->scene().add_object(renderer::SceneObject{
      .model = make_rc_name(resource.view().to_std()),
      .name = "New Object"_str,
      .transform = transform,
   });

   m_state.editor->set_selected_object(object_id);
   m_state.editor->viewport().update_view();
}

}// namespace triglav::editor
