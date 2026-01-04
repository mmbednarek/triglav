#include "RootWidget.hpp"

#include "DefaultRenderOverlay.hpp"
#include "Editor.hpp"
#include "ProjectExplorer.hpp"
#include "level_editor/LevelEditor.hpp"

#include "triglav/desktop_ui/MenuBar.hpp"
#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/desktop_ui/SecondaryEventGenerator.hpp"
#include "triglav/desktop_ui/Splitter.hpp"
#include "triglav/desktop_ui/TabView.hpp"

namespace triglav::editor {

using namespace name_literals;
using namespace string_literals;

using desktop::Key;
using desktop::Modifier;

class DefaultTabWidget final : public ui_core::IWidget
{
 public:
   explicit DefaultTabWidget(RootWindow& root_window) :
       m_root_window(root_window)
   {
   }

   [[nodiscard]] Vector2 desired_size(const Vector2 available_size) const override
   {
      return available_size;
   }

   void add_to_viewport(const Vector4 dimensions, Vector4 /*cropping_mask*/) override
   {
      m_default_overlay.set_dimensions(dimensions);
      m_root_window.set_render_overlay(&m_default_overlay);
   }

   void remove_from_viewport() override
   {
      m_root_window.set_render_overlay(nullptr);
   }

 private:
   RootWindow& m_root_window;
   DefaultRenderOverlay m_default_overlay;
};

RootWidget::RootWidget(ui_core::Context& context, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(context, parent),
    m_state(state),
    m_desktop_ui_manager(desktop_ui::ThemeProperties::get_default(), m_state.dialog_manager->root_surface(), *m_state.dialog_manager),
    TG_CONNECT(m_menu_bar_controller, OnClicked, on_clicked_menu_bar)
{
   auto& event_gen = this->emplace_content<desktop_ui::SecondaryEventGenerator>(m_context, this, m_state.dialog_manager->root_surface());
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

   m_command_manager.register_command("file.save"_name, {Modifier::Control, Key::S}, Command::Save);
   m_command_manager.register_command("file.close"_name, {Modifier::Control, Key::Q}, Command::Quit);
   m_command_manager.register_command("edit.undo"_name, {Modifier::Control, Key::Z}, Command::Undo);
   m_command_manager.register_command("edit.redo"_name, {Modifier::Control, Key::Y}, Command::Redo);
   m_command_manager.register_command("edit.delete"_name, {Modifier::Empty, Key::Delete}, Command::Delete);

   m_command_manager.register_command({Modifier::Empty, Key::G}, Command::TranslateTool);
   m_command_manager.register_command({Modifier::Empty, Key::R}, Command::RotationTool);
   m_command_manager.register_command({Modifier::Empty, Key::S}, Command::ScaleTool);
   m_command_manager.register_command({Modifier::Empty, Key::E}, Command::SelectionTool);

   m_menu_bar = &global_layout.create_child<desktop_ui::MenuBar>({
      .manager = &m_desktop_ui_manager,
      .controller = &m_menu_bar_controller,
   });

   auto& splitter = global_layout.create_child<desktop_ui::Splitter>({
      .manager = &m_desktop_ui_manager,
      .offset = 260,
      .axis = ui_core::Axis::Horizontal,
      .offset_type = desktop_ui::SplitterOffsetType::Following,
   });

   m_tab_view = &splitter.create_preceding<desktop_ui::TabView>({
      .manager = &m_desktop_ui_manager,
      .active_tab = 0,
   });
   TG_CONNECT_OPT(*m_tab_view, OnChangedActiveTab, on_changed_active_tab);

   m_tab_view->set_default_widget(std::make_unique<DefaultTabWidget>(*state.editor->root_window()));

   splitter.create_following<ProjectExplorer>({
      .manager = &m_desktop_ui_manager,
      .root_window = m_state.editor->root_window(),
   });
}

void RootWidget::on_clicked_menu_bar(const Name item_name, const desktop_ui::MenuItem& /*item*/) const
{
   const auto command = m_command_manager.translate_menu_item(item_name);
   if (command.has_value()) {
      this->on_command(command.value());
   }
}

void RootWidget::tick(const float delta_time) const
{
   if (m_active_asset_editor != nullptr) {
      m_active_asset_editor->tick(delta_time);
   }
}

void RootWidget::on_command(const Command command) const
{
   if (command == Command::Quit) {
      m_state.editor->close();
      return;
   }

   if (m_active_asset_editor != nullptr) {
      m_active_asset_editor->on_command(command);
   }
}

void RootWidget::on_event(const ui_core::Event& event)
{
   if (ui_core::visit_event<bool>(*this, event, true)) {
      ProxyWidget::on_event(event);
   }
}

bool RootWidget::on_key_pressed(const ui_core::Event& /*event*/, const ui_core::Event::Keyboard& keyboard) const
{
   if (m_active_asset_editor != nullptr && !m_active_asset_editor->accepts_key_chords())
      return true;

   const auto command = m_command_manager.translate_chord({m_state.dialog_manager->root_surface().modifiers(), keyboard.key});
   if (command.has_value()) {
      this->on_command(command.value());
      return false;
   }
   return true;
}

void RootWidget::on_changed_active_tab(u32 /*tab_id*/, ui_core::IWidget* widget)
{
   m_active_asset_editor = dynamic_cast<IAssetEditor*>(widget);
}

Vector4 RootWidget::asset_editor_area() const
{
   return m_tab_view->content_area();
}

void RootWidget::open_asset_editor(const ResourceName asset_name)
{
   // check if already opened
   for (u32 i = 0; i < m_tab_view->tab_count(); i++) {
      const auto& asset_editor = dynamic_cast<IAssetEditor&>(m_tab_view->tab(i).widget());
      if (asset_editor.asset_name() == asset_name) {
         m_tab_view->set_active_tab(i);
         return;
      }
   }

   // not found create new tab
   m_tab_view->remove_from_viewport();
   const auto tab_id = m_tab_view->add_tab(std::make_unique<LevelEditor>(m_context,
                                                                         LevelEditor::State{
                                                                            .manager = &m_desktop_ui_manager,
                                                                            .root_window = m_state.editor->root_window(),
                                                                            .asset_name = asset_name,
                                                                         },
                                                                         m_tab_view));
   m_tab_view->set_active_tab(tab_id);
}

}// namespace triglav::editor