#include "Splitter.hpp"

#include <DesktopUI.hpp>
#include <DialogManager.hpp>

namespace triglav::desktop_ui {

constexpr float g_splitterSize = 10.0f;

namespace {

desktop::CursorIcon axis_to_cursor_icon(const ui_core::Axis axis)
{
   switch (axis) {
   case ui_core::Axis::Horizontal:
      return desktop::CursorIcon::ResizeHorizontal;
   case ui_core::Axis::Vertical:
      return desktop::CursorIcon::ResizeVertical;
   }
   return desktop::CursorIcon::Arrow;
}

}// namespace

Splitter::Splitter(ui_core::Context& ctx, const State state, ui_core::IWidget* parent) :
    BaseWidget(parent),
    m_context(ctx),
    m_state(state)
{
}

Vector2 Splitter::desired_size(const Vector2 parentSize) const
{
   const auto preceding_size = m_preceding->desired_size(make_vec(m_state.offset, ortho(parentSize)));
   const auto following_size =
      m_following->desired_size(make_vec(parallel(parentSize) - g_splitterSize - m_state.offset, ortho(parentSize)));
   return make_vec(parallel(preceding_size) + parallel(following_size) + g_splitterSize,
                   std::max(ortho(preceding_size), ortho(following_size)));
}

void Splitter::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_dimensions = dimensions;
   m_croppingMask = croppingMask;

   const auto offset = rect_position(dimensions);
   const auto size = rect_size(dimensions);
   Vector2 preceding_size = make_vec(m_state.offset, ortho(size));
   Vector2 following_size = make_vec(parallel(size) - g_splitterSize - m_state.offset, ortho(size));
   Vector2 following_offset = make_vec(parallel(offset) + g_splitterSize + m_state.offset, ortho(offset));

   m_preceding->add_to_viewport({offset, preceding_size}, croppingMask);
   m_following->add_to_viewport({following_offset, following_size}, croppingMask);
}

void Splitter::remove_from_viewport()
{
   m_preceding->remove_from_viewport();
   m_following->remove_from_viewport();
}

void Splitter::on_event(const ui_core::Event& event)
{
   const float mouse_pos = parallel(event.mousePosition);

   if (m_isMoving) {
      if (event.eventType == ui_core::Event::Type::MouseMoved) {
         m_state.offset += mouse_pos - m_lastMousePos;
         m_lastMousePos = mouse_pos;
         this->add_to_viewport(m_dimensions, m_croppingMask);
      } else if (event.eventType == ui_core::Event::Type::MouseReleased) {
         m_isMoving = false;
      }
   } else if (m_isShowingCursor && (mouse_pos < m_state.offset || mouse_pos > m_state.offset + g_splitterSize)) {
      m_state.manager->dialog_manager().root().surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_isShowingCursor = false;
   }

   if (mouse_pos < m_state.offset) {
      m_preceding->on_event(event);
   } else if (mouse_pos > m_state.offset + g_splitterSize) {
      ui_core::Event sub_event{event};
      parallel(sub_event.mousePosition) -= m_state.offset + g_splitterSize;
      m_following->on_event(sub_event);
   } else {
      switch (event.eventType) {
      case ui_core::Event::Type::MouseMoved: {
         m_state.manager->dialog_manager().root().surface().set_cursor_icon(axis_to_cursor_icon(m_state.axis));
         m_isShowingCursor = true;
         break;
      }
      case ui_core::Event::Type::MousePressed: {
         m_isMoving = true;
         m_lastMousePos = mouse_pos;
         break;
      }
      default:
         break;
      }
   }
}

ui_core::IWidget& Splitter::set_preceding(ui_core::IWidgetPtr&& widget)
{
   m_preceding = std::move(widget);
   return *m_preceding;
}

ui_core::IWidget& Splitter::set_following(ui_core::IWidgetPtr&& widget)
{
   m_following = std::move(widget);
   return *m_following;
}

}// namespace triglav::desktop_ui
