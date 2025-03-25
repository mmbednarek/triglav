#include "widget/AlignmentBox.hpp"

namespace triglav::ui_core {

namespace {

Vector2 calculate_content_offset(const Alignment alignment, const Vector2 parentSize, const Vector2 contentSize)
{
   switch (alignment) {
   case Alignment::TopLeft:
      return {0, 0};
   case Alignment::TopCenter:
      return {0.5f * parentSize.x - 0.5f * contentSize.x, 0};
   case Alignment::TopRight:
      return {parentSize.x - contentSize.x, 0};
   case Alignment::Left:
      return {0, 0.5f * parentSize.y - 0.5f * contentSize.y};
   case Alignment::Center:
      return {0.5f * parentSize.x - 0.5f * contentSize.x, 0.5f * parentSize.y - 0.5f * contentSize.y};
   case Alignment::Right:
      return {parentSize.x - contentSize.x, 0.5f * parentSize.y - 0.5f * contentSize.y};
   case Alignment::BottomLeft:
      return {0, parentSize.y - contentSize.y};
   case Alignment::BottomCenter:
      return {0.5f * parentSize.x - 0.5f * contentSize.x, parentSize.y - contentSize.y};
   case Alignment::BottomRight:
      return {parentSize.x - contentSize.x, parentSize.y - contentSize.y};
   }

   return parentSize;
}

}// namespace

AlignmentBox::AlignmentBox(Context& ctx, State state, IWidget* parent) :
    m_context(ctx),
    m_state(std::move(state)),
    m_parent(parent)
{
}

Vector2 AlignmentBox::desired_size(const Vector2 parentSize) const
{
   return parentSize;
}

void AlignmentBox::add_to_viewport(const Vector4 dimensions)
{
   const Vector2 parentSize{dimensions.z, dimensions.w};
   const Vector2 size = m_content->desired_size(parentSize);
   const Vector2 offset = calculate_content_offset(m_state.alignment, parentSize, size);
   m_content->add_to_viewport({dimensions.x + offset.x, dimensions.y + offset.y, size.x, size.y});
   m_parentDimensions = dimensions;
}

void AlignmentBox::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void AlignmentBox::on_child_state_changed(IWidget& /*widget*/)
{
   m_content->add_to_viewport(m_parentDimensions);
}

IWidget& AlignmentBox::set_content(IWidgetPtr&& content)
{
   m_content = std::move(content);
   return *m_content;
}

void AlignmentBox::on_mouse_click(const desktop::MouseButton mouseButton, const Vector2 parentSize, const Vector2 position)
{
   const Vector2 size = m_content->desired_size(parentSize);
   const Vector2 offset = calculate_content_offset(m_state.alignment, parentSize, size);
   if (position.x > offset.x && position.x < (offset.x + size.x) && position.y > offset.y && position.y < (offset.y + size.y)) {
      m_content->on_mouse_click(mouseButton, size, position - offset);
   }
}

}// namespace triglav::ui_core
