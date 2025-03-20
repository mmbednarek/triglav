#include "DebugWidget.hpp"

#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::renderer {

using namespace name_literals;

DebugWidget::DebugWidget(ui_core::Context& ctx) :
    m_content(ctx, ui_core::RectBox::State{.color{0.0, 0.0, 0.5, 0.8}})
{
   auto& alignmentBox =
      m_content.emplace_content<ui_core::AlignmentBox>(ctx, ui_core::AlignmentBox::State{.alignment = ui_core::Alignment::BottomRight});
   auto& verticalLayout = alignmentBox.emplace_content<ui_core::VerticalLayout>(ctx, ui_core::VerticalLayout::State{
                                                                                        .padding{20.0f, 20.0f, 20.0f, 20.0f},
                                                                                        .separation = 25.0f,
                                                                                     });

   verticalLayout.emplace_child<ui_core::TextBox>(ctx, ui_core::TextBox::State{
                                                          .fontSize = 16,
                                                          .typeface = "cantarell.typeface"_rc,
                                                          .content = "Lorem ipsum",
                                                          .color = {1, 1, 1, 1},
                                                       });

   auto& horizontal = verticalLayout.emplace_child<ui_core::HorizontalLayout>(ctx, ui_core::HorizontalLayout::State{
                                                                                      .padding = {},
                                                                                      .separation = 10.0f,
                                                                                   });

   horizontal.emplace_child<ui_core::TextBox>(ctx, ui_core::TextBox::State{
                                                      .fontSize = 15,
                                                      .typeface = "cantarell.typeface"_rc,
                                                      .content = "beta delta",
                                                      .color = {0, 1, 1, 1},
                                                   });

   horizontal.emplace_child<ui_core::TextBox>(ctx, ui_core::TextBox::State{
                                                      .fontSize = 15,
                                                      .typeface = "cantarell.typeface"_rc,
                                                      .content = "beta delta",
                                                      .color = {1, 1, 0, 1},
                                                   });

   verticalLayout.emplace_child<ui_core::TextBox>(ctx, ui_core::TextBox::State{
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

void DebugWidget::add_to_viewport(const Vector4 dimensions)
{
   m_content.add_to_viewport(dimensions);
}

void DebugWidget::remove_from_viewport()
{
   m_content.remove_from_viewport();
}

}// namespace triglav::renderer
