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
   auto& alignment_box = m_content.create_content<ui_core::AlignmentBox>({
      .horizontal_alignment = ui_core::HorizontalAlignment::Right,
      .vertical_alignment = ui_core::VerticalAlignment::Bottom,
   });
   auto& vertical_layout = alignment_box.create_content<ui_core::VerticalLayout>({
      .padding{20.0f, 20.0f, 20.0f, 20.0f},
      .separation = 25.0f,
   });

   vertical_layout.create_child<ui_core::TextBox>({
      .font_size = 16,
      .typeface = "fonts/cantarell/regular.typeface"_rc,
      .content = "Lorem ipsum",
      .color = {1, 1, 1, 1},
   });

   auto& horizontal = vertical_layout.create_child<ui_core::HorizontalLayout>({
      .padding = {},
      .separation = 10.0f,
   });

   horizontal.create_child<ui_core::TextBox>({
      .font_size = 15,
      .typeface = "fonts/cantarell/regular.typeface"_rc,
      .content = "beta delta",
      .color = {0, 1, 1, 1},
   });

   horizontal.create_child<ui_core::TextBox>({
      .font_size = 15,
      .typeface = "fonts/cantarell/regular.typeface"_rc,
      .content = "beta delta",
      .color = {1, 1, 0, 1},
   });

   vertical_layout.create_child<ui_core::TextBox>({
      .font_size = 16,
      .typeface = "fonts/cantarell/regular.typeface"_rc,
      .content = "bar foo bar",
      .color = {0, 0, 1, 1},
   });
}

Vector2 DebugWidget::desired_size(const Vector2 parent_size) const
{
   return m_content.desired_size(parent_size);
}

void DebugWidget::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_content.add_to_viewport(dimensions, cropping_mask);
}

void DebugWidget::remove_from_viewport()
{
   m_content.remove_from_viewport();
}

}// namespace triglav::renderer
