#include "RootWidget.hpp"

#include "Editor.hpp"
#include "ProjectExplorer.hpp"
#include "level_editor/LevelEditor.hpp"

#include "triglav/desktop_ui/MenuBar.hpp"
#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/desktop_ui/SecondaryEventGenerator.hpp"
#include "triglav/desktop_ui/Splitter.hpp"
#include "triglav/desktop_ui/TabView.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace string_literals;

RootWidget::RootWidget(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state),
    m_desktop_uimanager(desktop_ui::ThemeProperties::get_default(), m_state.dialog_manager->root_surface(), *m_state.dialog_manager),
    TG_CONNECT(m_menu_bar_controller, OnClicked, on_clicked_menu_bar)
{
   auto& event_gen = this->emplace_content<desktop_ui::SecondaryEventGenerator>(m_context, this);
   auto& global_layout = event_gen.create_content<ui_core::VerticalLayout>({
      .padding = {},
      .separation = 0.0f,
   });

   m_menu_bar_controller.add_submenu("file"_name, "File"_strv);
   m_menu_bar_controller.add_subitem("file"_name, "file.save"_name, "Save"_strv);
   m_menu_bar_controller.add_subitem("file"_name, "file.save_all"_name, "Save All"_strv);
   m_menu_bar_controller.add_subitem("file"_name, "file.import"_name, "Import Asset"_strv);
   m_menu_bar_controller.add_seperator("file"_name);
   m_menu_bar_controller.add_subitem("file"_name, "file.properties"_name, "Project Properties"_strv);
   m_menu_bar_controller.add_subitem("file"_name, "file.close"_name, "Close"_strv);

   m_menu_bar_controller.add_submenu("edit"_name, "Edit"_strv);
   m_menu_bar_controller.add_subitem("edit"_name, "edit.undo"_name, "Undo"_strv);
   m_menu_bar_controller.add_subitem("edit"_name, "edit.redo"_name, "Redo"_strv);
   m_menu_bar_controller.add_seperator("edit"_name);
   m_menu_bar_controller.add_subitem("edit"_name, "edit.duplicate"_name, "Duplicate"_strv);
   m_menu_bar_controller.add_subitem("edit"_name, "edit.cut"_name, "Cut"_strv);
   m_menu_bar_controller.add_subitem("edit"_name, "edit.delete"_name, "Delete"_strv);

   m_menu_bar_controller.add_submenu("help"_name, "Help"_strv);
   m_menu_bar_controller.add_subitem("help"_name, "help.repository"_name, "Github Repository"_strv);
   m_menu_bar_controller.add_subitem("help"_name, "help.about"_name, "About"_strv);

   m_menu_bar = &global_layout.create_child<desktop_ui::MenuBar>({
      .manager = &m_desktop_uimanager,
      .controller = &m_menu_bar_controller,
   });

   auto& splitter = global_layout.create_child<desktop_ui::Splitter>({
      .manager = &m_desktop_uimanager,
      .offset = 260,
      .axis = ui_core::Axis::Horizontal,
      .offset_type = desktop_ui::SplitterOffsetType::Following,
   });

   auto& left_tab_view = splitter.create_preceding<desktop_ui::TabView>({
      .manager = &m_desktop_uimanager,
      .tab_names = {"Level Editor"},
      .active_tab = 0,
   });
   m_level_editor = &left_tab_view.create_child<LevelEditor>({
      .manager = &m_desktop_uimanager,
      .root_window = m_state.editor->root_window(),
   });

   auto& right_tab_view = splitter.create_following<desktop_ui::TabView>({
      .manager = &m_desktop_uimanager,
      .tab_names = {"Project Explorer"},
      .active_tab = 0,
   });
   right_tab_view.create_child<ProjectExplorer>({
      .manager = &m_desktop_uimanager,
   });
}

void RootWidget::on_clicked_menu_bar(const Name item_name, const desktop_ui::MenuItem& /*item*/) const
{
   switch (item_name) {
   case "file.close"_name:
      m_state.editor->close();
      break;
   case "edit.undo"_name: {
      ui_core::Event event{};
      event.event_type = ui_core::Event::Type::Undo;
      m_level_editor->on_event(event);
      break;
   }
   case "edit.redo"_name: {
      ui_core::Event event{};
      event.event_type = ui_core::Event::Type::Redo;
      m_level_editor->on_event(event);
      break;
   }
   default:
      break;
   }
}

void RootWidget::tick(const float delta_time) const
{
   assert(m_level_editor != nullptr);
   m_level_editor->tick(delta_time);
}

}// namespace triglav::editor