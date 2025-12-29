#include "Splitter.hpp"

#include "DesktopUI.hpp"
#include "PopupManager.hpp"

namespace triglav::desktop_ui {

constexpr float g_splitter_size = 6.0f;

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

float calculate_offset(const SplitterOffsetType type, const float offset, const ui_core::Axis axis, const Vector2 size)
{
   switch (type) {
   case SplitterOffsetType::Preceeding:
      return offset;
   case SplitterOffsetType::Following: {
      const auto dim = ui_core::parallel(size, axis);
      return dim - offset - g_splitter_size;
   }
   }
   return 0.0f;
}

}// namespace

Splitter::Splitter(ui_core::Context& ctx, const State state, ui_core::IWidget* parent) :
    BaseWidget(parent),
    m_context(ctx),
    m_state(state)
{
}

Vector2 Splitter::desired_size(const Vector2 available_size) const
{
   const float offset = calculate_offset(m_state.offset_type, m_state.offset, m_state.axis, available_size);
   const auto preceding_size = m_preceding->desired_size(make_vec(offset, ortho(available_size)));
   const auto following_size =
      m_following->desired_size(make_vec(parallel(available_size) - g_splitter_size - offset, ortho(available_size)));
   return make_vec(parallel(preceding_size) + parallel(following_size) + g_splitter_size,
                   std::max(ortho(preceding_size), ortho(following_size)));
}

void Splitter::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_dimensions = dimensions;
   m_cropping_mask = cropping_mask;

   const auto offset = rect_position(dimensions);
   const auto size = rect_size(dimensions);
   Vector2 preceding_size = make_vec(this->offset(), ortho(size));
   Vector2 following_size = make_vec(parallel(size) - g_splitter_size - this->offset(), ortho(size));
   Vector2 following_offset = make_vec(parallel(offset) + g_splitter_size + this->offset(), ortho(offset));

   Vector4 splitter_dims{make_vec(parallel(offset) + this->offset(), ortho(offset)), make_vec(g_splitter_size, ortho(size))};
   if (m_background == 0) {
      m_background = m_context.viewport().add_rectangle({
         .rect = splitter_dims,
         .color = TG_THEME_VAL(background_color_darker),
         .border_radius = {0, 0, 0, 0},
         .border_color = palette::NO_COLOR,
         .crop = m_cropping_mask,
         .border_width = 0.0f,
      });
   } else {
      m_context.viewport().set_rectangle_dims(m_background, splitter_dims, m_cropping_mask);
   }

   m_preceding->add_to_viewport({offset, preceding_size}, cropping_mask);
   m_following->add_to_viewport({following_offset, following_size}, cropping_mask);
}

void Splitter::remove_from_viewport()
{
   m_preceding->remove_from_viewport();
   m_following->remove_from_viewport();
}

void Splitter::on_event(const ui_core::Event& event)
{
   const float mouse_pos = parallel(event.mouse_position);

   if (m_is_moving) {
      if (event.event_type == ui_core::Event::Type::MouseMoved) {
         this->add_offset(mouse_pos - m_last_mouse_pos);
         m_last_mouse_pos = mouse_pos;
         this->add_to_viewport(m_dimensions, m_cropping_mask);
      } else if (event.event_type == ui_core::Event::Type::MouseReleased) {
         m_is_moving = false;
      }
   } else if (m_is_showing_cursor && (mouse_pos < this->offset() || mouse_pos > this->offset() + g_splitter_size)) {
      m_state.manager->popup_manager().root_surface().set_cursor_icon(desktop::CursorIcon::Arrow);
      m_is_showing_cursor = false;
   }

   if (mouse_pos < this->offset()) {
      m_preceding->on_event(event);
   } else if (mouse_pos > this->offset() + g_splitter_size) {
      ui_core::Event sub_event{event};
      parallel(sub_event.mouse_position) -= this->offset() + g_splitter_size;
      m_following->on_event(sub_event);
   } else {
      switch (event.event_type) {
      case ui_core::Event::Type::MouseMoved: {
         m_state.manager->popup_manager().root_surface().set_cursor_icon(axis_to_cursor_icon(m_state.axis));
         m_is_showing_cursor = true;
         break;
      }
      case ui_core::Event::Type::MousePressed: {
         m_is_moving = true;
         m_last_mouse_pos = mouse_pos;
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

float Splitter::offset() const
{
   return calculate_offset(m_state.offset_type, m_state.offset, m_state.axis, rect_size(m_dimensions));
}

void Splitter::add_offset(const float diff)
{
   switch (m_state.offset_type) {
   case SplitterOffsetType::Preceeding:
      m_state.offset += diff;
      break;
   case SplitterOffsetType::Following: {
      m_state.offset -= diff;
      break;
   }
   }
}

}// namespace triglav::desktop_ui
