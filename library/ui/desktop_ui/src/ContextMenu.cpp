#include "ContextMenu.hpp"

#include "PopupManager.hpp"

namespace triglav::desktop_ui {

using namespace name_literals;

ContextMenu::ContextMenu(ui_core::Context& ctx, const State state, ui_core::IWidget* parent) :
    ui_core::ContainerWidget(ctx, parent),
    m_state(state),
    TG_CONNECT(*m_state.controller, OnClicked, on_clicked)
{
}

Vector2 ContextMenu::desired_size(const Vector2 parent_size) const
{
   return m_content->desired_size(parent_size);
}

void ContextMenu::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   return m_content->add_to_viewport(dimensions, cropping_mask);
}

void ContextMenu::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void ContextMenu::on_event(const ui_core::Event& event)
{
   if (event.event_type == ui_core::Event::Type::MousePressed) {
      const auto mouse_payload = std::get<ui_core::Event::Mouse>(event.data);
      if (m_menu_dialog != nullptr) {
         m_state.manager->popup_manager().close_popup(m_menu_dialog);
         m_menu_dialog = nullptr;
      } else if (mouse_payload.button == desktop::MouseButton::Right) {
         MenuList::State child_state{
            .manager = m_state.manager,
            .controller = m_state.controller,
            .list_name = "root"_name,
            .screen_offset = event.global_mouse_position,
         };
         const auto temporary_menu = std::make_unique<MenuList>(m_context, child_state, nullptr);
         const auto size = temporary_menu->desired_size({});

         auto& popup = m_state.manager->popup_manager().create_popup_dialog(event.global_mouse_position, size);
         popup.create_root_widget<MenuList>(MenuList::State{child_state});
         popup.initialize();
         m_menu_dialog = &popup;
         return;
      }
   }
   m_content->on_event(event);
}

void ContextMenu::on_clicked(Name /*name*/, const MenuItem& /*item*/)
{
   m_state.manager->popup_manager().close_popup(m_menu_dialog);
   m_menu_dialog = nullptr;
}

}// namespace triglav::desktop_ui
