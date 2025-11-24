#pragma once

#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/TreeView.hpp"
#include "triglav/renderer/Scene.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {
class DesktopUIManager;
}

namespace triglav::editor {

class LevelEditor;

using namespace name_literals;

class SceneView final : public ui_core::ProxyWidget
{
 public:
   using Self = SceneView;

   struct State
   {
      desktop_ui::DesktopUIManager* manager{};
      LevelEditor* editor{};
   };

   SceneView(ui_core::Context& context, State state, IWidget* parent);

   void on_selected_object(const desktop_ui::TreeItemId item_id);
   void on_clicked_add();
   void on_clicked_add_directory();
   void on_clicked_delete();
   void on_object_added_to_scene(const renderer::ObjectID object_id, const renderer::SceneObject& object);
   void on_object_is_removed(const renderer::ObjectID object_id) const;

   void update_selected_item() const;

 private:
   State m_state;
   desktop_ui::TreeController m_tree_controller;
   desktop_ui::TreeView* m_tree_view;

   std::map<desktop_ui::TreeItemId, renderer::ObjectID> m_item_id_to_object_id;
   std::map<renderer::ObjectID, desktop_ui::TreeItemId> m_object_id_to_item_id;

   TG_SINK(renderer::Scene, OnObjectAddedToScene);
   TG_OPT_SINK(desktop_ui::TreeView, OnSelected);
   TG_OPT_NAMED_SINK(desktop_ui::Button, OnClick, Add);
   TG_OPT_NAMED_SINK(desktop_ui::Button, OnClick, AddDirectory);
   TG_OPT_NAMED_SINK(desktop_ui::Button, OnClick, Delete);
};

}// namespace triglav::editor
