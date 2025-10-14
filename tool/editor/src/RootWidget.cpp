#include "RootWidget.hpp"

#include "Editor.hpp"

#include "triglav/desktop_ui/DialogManager.hpp"
#include "triglav/desktop_ui/MenuBar.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace string_literals;

RootWidget::RootWidget(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ui_core::BaseWidget(parent),
    m_context(context),
    m_state(state),
    m_globalLayout(m_context,
                   {
                      .padding = {},
                      .separation = 0.0f,
                   },
                   this),
    m_deskopUIManager(desktop_ui::ThemeProperties::get_default(), m_state.dialogManager->root().surface(), *m_state.dialogManager),
    TG_CONNECT(m_menuBarController, OnClicked, on_clicked_menu_bar)
{
   m_menuBarController.add_submenu("file"_name, "File"_strv);
   m_menuBarController.add_subitem("file"_name, "file.import"_name, "Import Asset"_strv);
   m_menuBarController.add_subitem("file"_name, "file.close"_name, "Close"_strv);

   m_menuBarController.add_submenu("edit"_name, "Edit"_strv);
   m_menuBarController.add_subitem("edit"_name, "edit.undo"_name, "Undo"_strv);
   m_menuBarController.add_subitem("edit"_name, "edit.redo"_name, "Redo"_strv);

   m_menuBarController.add_submenu("help"_name, "Help"_strv);
   m_menuBarController.add_subitem("help"_name, "help.repository"_name, "Github Repository"_strv);
   m_menuBarController.add_subitem("help"_name, "help.about"_name, "About"_strv);

   m_menuBar = &m_globalLayout.create_child<desktop_ui::MenuBar>({
      .manager = &m_deskopUIManager,
      .controller = &m_menuBarController,
   });
}

Vector2 RootWidget::desired_size(Vector2 parentSize) const
{
   return m_globalLayout.desired_size(parentSize);
}

void RootWidget::add_to_viewport(Vector4 dimensions, Vector4 croppingMask)
{
   m_globalLayout.add_to_viewport(dimensions, croppingMask);
}

void RootWidget::remove_from_viewport()
{
   m_globalLayout.remove_from_viewport();
}

void RootWidget::on_event(const ui_core::Event& event)
{
   m_globalLayout.on_event(event);
}

void RootWidget::on_clicked_menu_bar(Name itemName, const desktop_ui::MenuItem& /*item*/) {
   if (itemName == "file.close"_name) {
      m_state.editor->close();
   }
}

}// namespace triglav::editor