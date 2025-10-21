#include "RootWidget.hpp"

#include "Editor.hpp"
#include "LevelEditor.hpp"
#include "ProjectExplorer.hpp"

#include "triglav/desktop_ui/MenuBar.hpp"
#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/desktop_ui/Splitter.hpp"
#include "triglav/desktop_ui/TabView.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace string_literals;

RootWidget::RootWidget(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_context(context),
    m_state(state),
    m_desktopUIManager(desktop_ui::ThemeProperties::get_default(), m_state.dialogManager->root_surface(), *m_state.dialogManager),
    TG_CONNECT(m_menuBarController, OnClicked, on_clicked_menu_bar)
{
   auto& globalLayout = this->create_content<ui_core::VerticalLayout>({
      .padding = {},
      .separation = 0.0f,
   });

   m_menuBarController.add_submenu("file"_name, "File"_strv);
   m_menuBarController.add_subitem("file"_name, "file.import"_name, "Import Asset"_strv);
   m_menuBarController.add_subitem("file"_name, "file.close"_name, "Close"_strv);

   m_menuBarController.add_submenu("edit"_name, "Edit"_strv);
   m_menuBarController.add_subitem("edit"_name, "edit.undo"_name, "Undo"_strv);
   m_menuBarController.add_subitem("edit"_name, "edit.redo"_name, "Redo"_strv);

   m_menuBarController.add_submenu("help"_name, "Help"_strv);
   m_menuBarController.add_subitem("help"_name, "help.repository"_name, "Github Repository"_strv);
   m_menuBarController.add_subitem("help"_name, "help.about"_name, "About"_strv);

   m_menuBar = &globalLayout.create_child<desktop_ui::MenuBar>({
      .manager = &m_desktopUIManager,
      .controller = &m_menuBarController,
   });

   auto& splitter = globalLayout.create_child<desktop_ui::Splitter>({
      .manager = &m_desktopUIManager,
      .offset = 300,
      .axis = ui_core::Axis::Horizontal,
      .offset_type = desktop_ui::SplitterOffsetType::Following,
   });

   auto& leftTabView = splitter.create_preceding<desktop_ui::TabView>({
      .manager = &m_desktopUIManager,
      .tabNames = {"Level Editor"},
      .activeTab = 0,
   });
   leftTabView.create_child<LevelEditor>({
      .manager = &m_desktopUIManager,
      .rootWindow = m_state.editor->root_window(),
   });

   auto& rightTabView = splitter.create_following<desktop_ui::TabView>({
      .manager = &m_desktopUIManager,
      .tabNames = {"Project Explorer"},
      .activeTab = 0,
   });
   rightTabView.create_child<ProjectExplorer>({
      .manager = &m_desktopUIManager,
   });
}

void RootWidget::on_clicked_menu_bar(const Name itemName, const desktop_ui::MenuItem& /*item*/) const
{
   if (itemName == "file.close"_name) {
      m_state.editor->close();
   }
}

}// namespace triglav::editor