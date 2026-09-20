#include "SceneView.hpp"

#include "../ResourceSelector.hpp"
#include "LevelEditor.hpp"
#include "src/UIWorldSystem.hpp"

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/engine/Engine.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/Padding.hpp"
#include "triglav/ui_core/widget/ScrollBox.hpp"

namespace triglav::editor {

using namespace string_literals;

namespace {

Vector4 get_icon(const world::EntityID entity_id)
{
   if (entity_id == world::ROOT_ENTITY) {
      return {7 * 18, 18, 18, 18};
   }
   return {5 * 18, 18, 18, 18};
}

}// namespace

SceneView::SceneView(ui_core::Context& context, const State state, ui_core::IWidget* parent) :
    desktop_ui::DesktopProxyWidget(context, parent),
    m_state(state),
    TG_CONNECT(state.editor->level().system<UIWorldSystem>(), OnAddedEntity, on_added_entity),
    TG_CONNECT(state.editor->level().system<UIWorldSystem>(), OnRemovedEntity, on_removed_entity),
    TG_CONNECT(state.editor->level().system<UIWorldSystem>(), OnModifiedLabel, on_modified_label)
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
      .suffix = ".mesh",
   });
   TG_CONNECT_NAMED_OPT(add_trigger, OnSelected, OnResourceSelected, on_resource_selected);

   auto& add_button = add_trigger.create_content<desktop_ui::Button>({});
   add_button.create_content<ui_core::Image>({
      .texture = "editor/texture/ui_icons.tex"_rc,
      .max_size = Vector2{22, 22},
      .region = Vector4{0, 2 * 22, 22, 22},
   });

   auto& add_dir_button = buttons.create_child<desktop_ui::Button>({});
   add_dir_button.create_content<ui_core::Image>({
      .texture = "editor/texture/ui_icons.tex"_rc,
      .max_size = Vector2{22, 22},
      .region = Vector4{22, 2 * 22, 22, 22},
   });
   TG_CONNECT_NAMED_OPT(add_dir_button, OnClick, AddDirectory, on_clicked_add_directory);

   auto& delete_button = buttons.create_child<desktop_ui::Button>({});
   delete_button.create_content<ui_core::Image>({
      .texture = "editor/texture/ui_icons.tex"_rc,
      .max_size = Vector2{22, 22},
      .region = Vector4{2 * 22, 2 * 22, 22, 22},
   });
   TG_CONNECT_NAMED_OPT(delete_button, OnClick, Delete, on_clicked_delete);

   m_tree_view = &vert_layout
                     .create_child<ui_core::RectBox>({
                        .color = TG_THEME_VAL(background_color_darker),
                        .border_radius = {4, 4, 4, 4},
                        .border_color = palette::NO_COLOR,
                        .border_width = 0.0f,
                     })
                     .create_content<ui_core::ScrollBox>({
                        .offset = 0.0f,
                     })
                     .create_content<desktop_ui::TreeView>({
                        .controller = &m_tree_controller,
                        .extended_items = {},
                     });
   TG_CONNECT_OPT(*m_tree_view, OnSelected, on_selected_object);

   state.editor->level().system<UIWorldSystem>().discover_entities(
      this, [](void* user, const world::EntityID parent, const world::EntityID child, const StringView label) {
         static_cast<SceneView*>(user)->on_added_entity(parent, child, label);
      });
}

void SceneView::on_added_entity(const world::EntityID parent, const world::EntityID child, const StringView label)
{
   this->add_entity(parent, child, label);
}

void SceneView::on_removed_entity(const world::EntityID entity_id) const
{
   m_tree_view->remove_item(m_object_id_to_item_id.at(entity_id));
}

void SceneView::on_modified_label(const world::EntityID entity_id, const StringView label) const
{
   m_tree_view->set_label(m_object_id_to_item_id.at(entity_id), label);
}

void SceneView::on_selected_object(const desktop_ui::TreeItemId item_id)
{
   const auto object_id = m_item_id_to_object_id[item_id];
   m_state.editor->set_selected_object(object_id);
}

void SceneView::on_clicked_add_directory()
{
   log_error("Adding directory is not implemented yet");
}

void SceneView::on_clicked_delete() const
{
   m_state.editor->remove_selected_item();
}

void SceneView::update_selected_item() const
{
   m_tree_view->set_selected_item(m_object_id_to_item_id.at(m_state.editor->selected_object_id()));
}

void SceneView::on_resource_selected(const String /*resource*/) const
{
   log_error("UNIMPLEMENTED");
}

void SceneView::add_entity(const world::EntityID parent, const world::EntityID child, const StringView label)
{
   desktop_ui::TreeItemId tree_parent = 0;
   if (parent != world::NO_ENTITY) {
      tree_parent = m_object_id_to_item_id.at(parent);
   }

   const auto tree_item_id = m_tree_controller.add_item(tree_parent, {
                                                                        .icon_name = "editor/texture/ui_icons.tex"_rc,
                                                                        .icon_region = get_icon(child),
                                                                        .label = label,
                                                                        .has_children = false,
                                                                     });
   m_item_id_to_object_id[tree_item_id] = child;
   m_object_id_to_item_id[child] = tree_item_id;
}

}// namespace triglav::editor
