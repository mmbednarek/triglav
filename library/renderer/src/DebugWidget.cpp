#include "DebugWidget.hpp"

#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::renderer {

using namespace name_literals;

DebugWidget::DebugWidget(ui_core::Context& ctx) :
    m_content(ctx, ui_core::RectBox::State{.color{0.0, 0.0, 0.5, 0.8}}, this)
{
   auto& alignmentBox = m_content.create_content<ui_core::AlignmentBox>({
      .horizontalAlignment = ui_core::HorizontalAlignment::Right,
      .verticalAlignment = ui_core::VerticalAlignment::Bottom,
   });
   auto& verticalLayout = alignmentBox.create_content<ui_core::VerticalLayout>({
      .padding{20.0f, 20.0f, 20.0f, 20.0f},
      .separation = 25.0f,
   });

   verticalLayout.create_child<ui_core::TextBox>({
      .fontSize = 16,
      .typeface = "cantarell.typeface"_rc,
      .content = "Lorem ipsum",
      .color = {1, 1, 1, 1},
   });

   auto& horizontal = verticalLayout.create_child<ui_core::HorizontalLayout>({
      .padding = {},
      .separation = 10.0f,
   });

   horizontal.create_child<ui_core::TextBox>({
      .fontSize = 15,
      .typeface = "cantarell.typeface"_rc,
      .content = "beta delta",
      .color = {0, 1, 1, 1},
   });

   horizontal.create_child<ui_core::TextBox>({
      .fontSize = 15,
      .typeface = "cantarell.typeface"_rc,
      .content = "beta delta",
      .color = {1, 1, 0, 1},
   });

   verticalLayout.create_child<ui_core::TextBox>({
      .fontSize = 16,
      .typeface = "cantarell.typeface"_rc,
      .content = "bar foo bar",
      .color = {0, 0, 1, 1},
   });
}

Vector2 DebugWidget::desired_size(const Vector2 parentSize) const
{
   return m_content.desired_size(parentSize);
}

void DebugWidget::add_to_viewport(const Vector4 dimensions, const Vector4 croppingMask)
{
   m_content.add_to_viewport(dimensions, croppingMask);
}

void DebugWidget::remove_from_viewport()
{
   m_content.remove_from_viewport();
}

}// namespace triglav::renderer
