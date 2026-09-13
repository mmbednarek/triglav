#pragma once

#include "../ResourceSelector.hpp"

#include "triglav/Logging.hpp"
#include "triglav/desktop_ui/Button.hpp"
#include "triglav/desktop_ui/Dialog.hpp"
#include "triglav/desktop_ui/TreeView.hpp"
#include "triglav/renderer/Scene.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {
class DesktopContext;
}

namespace triglav::editor {

class LevelEditor;

using namespace name_literals;

class SceneView final : public desktop_ui::DesktopProxyWidget
{
   TG_DEFINE_LOG_CATEGORY(SceneView)
 public:
   TG_TAG_CLASS(triglav::editor::SceneView)

   struct State
   {
      LevelEditor* editor{};
   };

   SceneView(ui_core::Context& context, State state, IWidget* parent);

   void on_added_entity(world::EntityID parent, world::EntityID child, StringView label);
   void on_removed_entity(world::EntityID entity_id) const;
   void on_modified_label(world::EntityID entity_id, StringView label) const;

   void on_selected_object(desktop_ui::TreeItemId item_id);
   void on_clicked_add_directory();
   void on_clicked_delete() const;

   void update_selected_item() const;
   void on_resource_selected(String resource) const;

 private:
   void add_entity(world::EntityID parent, world::EntityID child, StringView label);

   State m_state;
   desktop_ui::TreeController m_tree_controller;
   desktop_ui::TreeView* m_tree_view;

   std::map<desktop_ui::TreeItemId, world::EntityID> m_item_id_to_object_id;
   std::map<world::EntityID, desktop_ui::TreeItemId> m_object_id_to_item_id;


   TG_SINK(OnAddedEntity);
   TG_SINK(OnRemovedEntity);
   TG_SINK(OnModifiedLabel);

   TG_SINK(OnSelected);
   TG_SINK(OnResourceSelected);
   TG_SINK(Add);
   TG_SINK(AddDirectory);
   TG_SINK(Delete);
};

}// namespace triglav::editor
