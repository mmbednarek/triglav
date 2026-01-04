#pragma once

#include "CommandManager.hpp"
#include "IAssetEditor.hpp"

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/MenuController.hpp"
#include "triglav/desktop_ui/TabView.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::desktop_ui {
class MenuBar;
}

namespace triglav::editor {

class Editor;
class LevelEditor;

class RootWidget final : public ui_core::ProxyWidget
{
 public:
   using Self = RootWidget;
   struct State
   {
      desktop_ui::PopupManager* dialog_manager;
      Editor* editor;
   };

   RootWidget(ui_core::Context& context, State state, ui_core::IWidget* parent);

   void on_clicked_menu_bar(Name item_name, const desktop_ui::MenuItem& item) const;
   void tick(float delta_time) const;
   void on_command(Command command) const;
   void on_event(const ui_core::Event& event) override;
   bool on_key_pressed(const ui_core::Event& event, const ui_core::Event::Keyboard& keyboard) const;
   void on_changed_active_tab(u32 tab_id, ui_core::IWidget* widget);
   [[nodiscard]] Vector4 asset_editor_area() const;
   void open_asset_editor(ResourceName asset_name);

 private:
   State m_state;
   desktop_ui::DesktopUIManager m_desktop_ui_manager;
   desktop_ui::MenuController m_menu_bar_controller;
   CommandManager m_command_manager;
   desktop_ui::MenuBar* m_menu_bar;
   IAssetEditor* m_active_asset_editor{};
   desktop_ui::TabView* m_tab_view;

   TG_SINK(desktop_ui::MenuController, OnClicked);
   TG_OPT_SINK(desktop_ui::TabView, OnChangedActiveTab);
};

}// namespace triglav::editor