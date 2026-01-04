#pragma once

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/TreeController.hpp"
#include "triglav/desktop_ui/TreeView.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/io/Path.hpp"
#include "triglav/ui_core/IWidget.hpp"

#include <map>

namespace triglav::editor {

class RootWindow;

class ProjectTreeController final : public desktop_ui::ITreeController
{
 public:
   void children(desktop_ui::TreeItemId parent, std::function<void(desktop_ui::TreeItemId)> callback) override;
   const desktop_ui::TreeItem& item(desktop_ui::TreeItemId id) override;
   void set_label(desktop_ui::TreeItemId id, StringView label) override;
   void remove(desktop_ui::TreeItemId /*id*/) override;
   io::Path path(desktop_ui::TreeItemId id);
   [[nodiscard]] bool has_children(desktop_ui::TreeItemId id) const;

 private:
   std::map<desktop_ui::TreeItemId, std::vector<desktop_ui::TreeItemId>> m_cache;
   std::map<desktop_ui::TreeItemId, io::Path> m_id_to_path;
   std::map<desktop_ui::TreeItemId, desktop_ui::TreeItem> m_id_to_item;
   desktop_ui::TreeItemId m_top_item = desktop_ui::TREE_ROOT + 1;
};

class ProjectExplorer final : public ui_core::ProxyWidget
{
 public:
   using Self = ProjectExplorer;
   struct State
   {
      desktop_ui::DesktopUIManager* manager;
      RootWindow* root_window;
   };

   ProjectExplorer(ui_core::Context& context, State state, ui_core::IWidget* parent);

   void on_activated(desktop_ui::TreeItemId id);

 private:
   State m_state;
   ProjectTreeController m_controller;
   std::map<std::string_view, u32> m_path_to_id;

   TG_OPT_SINK(desktop_ui::TreeView, OnActivated);
};

}// namespace triglav::editor
