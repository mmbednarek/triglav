#include "ContextMenu.hpp"

#include "DialogManager.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

ContextMenu::ContextMenu(ui_core::Context& ctx, const State state, ui_core::IWidget* parent) :
    ui_core::ContainerWidget(ctx, parent),
    m_state(state),
    TG_CONNECT(*m_state.controller, OnClicked, on_clicked)
{
}

Vector2 ContextMenu::desired_size(const Vector2 parentSize) const
{
   return m_content->desired_size(parentSize);
}

void ContextMenu::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   return m_content->add_to_viewport(dimensions, croppingMask);
}

void ContextMenu::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void ContextMenu::on_event(const ui_core::Event& event)
{
   if (event.eventType == ui_core::Event::Type::MousePressed) {
      const auto mouse_payload = std::get<ui_core::Event::Mouse>(event.data);
      if (m_menuDialog != nullptr) {
         m_state.manager->dialog_manager().close_popup(m_menuDialog);
         m_menuDialog = nullptr;
      } else if (mouse_payload.button == desktop::MouseButton::Right) {
         const auto size = MenuList::calculate_size(*m_state.controller, "root"_name);

         auto& popup = m_state.manager->dialog_manager().create_popup_dialog(event.globalMousePosition, size);
         popup.create_root_widget<MenuList>({
            .manager = m_state.manager,
            .controller = m_state.controller,
            .listName = "root"_name,
            .screenOffset = event.globalMousePosition,
         });
         popup.initialize();
         m_menuDialog = &popup;
         return;
      }
   }
   m_content->on_event(event);
}

void ContextMenu::on_clicked(Name /*name*/, const MenuItem& /*item*/)
{
   m_state.manager->dialog_manager().close_popup(m_menuDialog);
   m_menuDialog = nullptr;
}

}// namespace triglav::desktop_ui
