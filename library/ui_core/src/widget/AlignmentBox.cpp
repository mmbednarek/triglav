#include "widget/AlignmentBox.hpp"

namespace triglav::ui_core {

namespace {

Vector4 align_content(const Alignment alignment, const Vector4 parentDim, const Vector2 contentSize)
{
   switch (alignment) {
   case Alignment::TopLeft:
      return {parentDim.x, parentDim.y, contentSize.x, contentSize.y};
   case Alignment::TopCenter:
      return {parentDim.x + 0.5f * parentDim.z - 0.5f * contentSize.x, parentDim.y, contentSize.x, contentSize.y};
   case Alignment::TopRight:
      return {parentDim.x + parentDim.z - contentSize.x, parentDim.y, contentSize.x, contentSize.y};
   case Alignment::Left:
      return {parentDim.x, parentDim.y + 0.5f * parentDim.w - 0.5f * contentSize.y, contentSize.x, contentSize.y};
   case Alignment::Center:
      return {parentDim.x + 0.5f * parentDim.z - 0.5f * contentSize.x, parentDim.y + 0.5f * parentDim.w - 0.5f * contentSize.y,
              contentSize.x, contentSize.y};
   case Alignment::Right:
      return {parentDim.x + parentDim.z - contentSize.x, parentDim.y + 0.5f * parentDim.w - 0.5f * contentSize.y, contentSize.x,
              contentSize.y};
   case Alignment::BottomLeft:
      return {parentDim.x, parentDim.y + parentDim.w - contentSize.y, contentSize.x, contentSize.y};
   case Alignment::BottomCenter:
      return {parentDim.x + 0.5f * parentDim.z - 0.5f * contentSize.x, parentDim.y + parentDim.w - contentSize.y, contentSize.x,
              contentSize.y};
   case Alignment::BottomRight:
      return {parentDim.x + parentDim.z - contentSize.x, parentDim.y + parentDim.w - contentSize.y, contentSize.x, contentSize.y};
   }

   return parentDim;
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

void AlignmentBox::add_to_viewport(Vector4 dimensions)
{
   const Vector2 size = m_content->desired_size({dimensions.z, dimensions.w});
   m_content->add_to_viewport(align_content(m_state.alignment, dimensions, size));
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

}// namespace triglav::ui_core
