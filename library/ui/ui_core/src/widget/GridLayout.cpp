#include "widget/GridLayout.hpp"

#include "Context.hpp"

namespace triglav::ui_core {

GridLayout::GridLayout(Context& context, State state, IWidget* parent) :
    LayoutWidget(context, parent),
    m_state(std::move(state))
{
}

Vector2 GridLayout::desired_size(const Vector2 parentSize) const
{
   const auto row_count = m_state.row_ratios.size();
   const auto column_count = m_state.column_ratios.size();
   assert(m_children.size() == row_count * column_count);

   const Vector2 base_size = parentSize - Vector2{static_cast<float>(column_count - 1) * m_state.horizontal_spacing,
                                                  static_cast<float>(row_count - 1) * m_state.vertical_spacing};

   Vector2 result{};
   for (MemorySize row = 0; row < row_count; ++row) {
      float x_offset = 0.0f;
      float max_height = 0.0f;
      for (MemorySize col = 0; col < column_count; ++col) {
         const auto& widget = *m_children[col + row * column_count];
         const auto size = widget.desired_size({base_size.x * m_state.column_ratios[col], base_size.y * m_state.row_ratios[row]});

         x_offset += size.x + m_state.horizontal_spacing;
         max_height = std::max(max_height, size.y);
      }
      result.x = std::max(result.x, x_offset);
      result.y += max_height + m_state.vertical_spacing;
   }

   return result - Vector2{m_state.horizontal_spacing, m_state.vertical_spacing};
}

void GridLayout::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_dimensions = dimensions;

   const auto row_count = m_state.row_ratios.size();
   const auto column_count = m_state.column_ratios.size();

   const Vector2 base_size = rect_size(dimensions) - Vector2{static_cast<float>(column_count - 1) * m_state.horizontal_spacing,
                                                             static_cast<float>(row_count - 1) * m_state.vertical_spacing};

   float y_offset = 0.0f;
   for (MemorySize row = 0; row < row_count; ++row) {
      float x_offset = 0.0f;
      float max_height = 0.0f;
      for (MemorySize col = 0; col < column_count; ++col) {
         auto& widget = *m_children[col + row * column_count];

         float width = base_size.x * m_state.column_ratios[col];
         float height = base_size.y * m_state.row_ratios[row];

         widget.add_to_viewport({dimensions.x + x_offset, dimensions.y + y_offset, width, height}, croppingMask);

         x_offset += width + m_state.horizontal_spacing;
         max_height = std::max(max_height, height);
      }

      y_offset += max_height + m_state.vertical_spacing;
   }
}

void GridLayout::remove_from_viewport()
{
   for (const auto& widget : m_children) {
      widget->remove_from_viewport();
   }
}

void GridLayout::on_event(const Event& event)
{
   if (event.eventType == Event::Type::MouseLeft) {
      for (const auto& child : m_children) {
         child->on_event(event);
      }
      return;
   }

   const auto row_count = m_state.row_ratios.size();
   const auto column_count = m_state.column_ratios.size();

   const Vector2 base_size = rect_size(m_dimensions) - Vector2{static_cast<float>(column_count - 1) * m_state.horizontal_spacing,
                                                               static_cast<float>(row_count - 1) * m_state.vertical_spacing};

   u32 row = 0, col = 0;
   float x_offset = 0.0f;
   for (const auto& ratio : m_state.column_ratios) {
      const float width = ratio * base_size.x;
      if (event.mousePosition.x >= x_offset && event.mousePosition.x < (x_offset + width)) {
         break;
      }
      x_offset += width + m_state.horizontal_spacing;
      ++col;
   }

   float y_offset = 0.0f;
   for (const auto& ratio : m_state.row_ratios) {
      const float height = ratio * base_size.y;
      if (event.mousePosition.y >= y_offset && event.mousePosition.y < (y_offset + height)) {
         break;
      }
      y_offset += height + m_state.vertical_spacing;
      ++row;
   }

   if (m_lastCol != col || m_lastRow != row) {
      Event leave_event{};
      leave_event.eventType = Event::Type::MouseLeft;
      leave_event.mousePosition = Vector2{0, 0};
      leave_event.parentSize = rect_size(m_dimensions);
      leave_event.globalMousePosition = event.globalMousePosition;

      const auto leave_index = m_lastCol + m_lastRow * column_count;
      if (leave_index < m_children.size()) {
         log_info("sending leave event to row: {}, col: {}", m_lastRow, m_lastCol);
         m_children[leave_index]->on_event(leave_event);
      }

      m_lastRow = row;
      m_lastCol = col;

      Event enter_event{};
      enter_event.eventType = Event::Type::MouseEntered;
      enter_event.mousePosition -= Vector2{x_offset, y_offset};
      enter_event.parentSize = rect_size(m_dimensions);
      enter_event.globalMousePosition = event.globalMousePosition;

      const auto enter_index = col + row * column_count;
      if (enter_index < m_children.size()) {
         log_info("sending enter event to row: {}, col: {}", row, col);
         m_children[enter_index]->on_event(enter_event);
      }
   }

   Event sub_event{event};
   sub_event.mousePosition -= Vector2{x_offset, y_offset};
   sub_event.parentSize = rect_size(m_dimensions);
   const auto index = col + row * column_count;
   if (index < m_children.size()) {
      m_children[index]->on_event(sub_event);
   }

   if (event.eventType == Event::Type::KeyPressed && std::get<Event::Keyboard>(event.data).key == desktop::Key::Tab) {
      if (!m_activeChild.has_value()) {
         m_activeChild = m_lastCol + m_state.column_ratios.size() * m_lastRow;
      }
      m_activeChild = (*m_activeChild + 1) % m_children.size();

      Event activate_event(event);
      activate_event.eventType = Event::Type::Activated;
      m_children[*m_activeChild]->on_event(activate_event);
   }
}

std::optional<MemorySize> GridLayout::find_active_child() const
{
   MemorySize index = 0;
   for (const auto& child : m_children) {
      if (child.get() == m_context.active_widget()) {
         return index;
      }
      ++index;
   }
   return std::nullopt;
}

}// namespace triglav::ui_core
