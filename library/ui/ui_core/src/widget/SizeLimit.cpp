#include "widget/SizeLimit.hpp"

namespace triglav::ui_core {

SizeLimit::SizeLimit(Context& ctx, State state, IWidget* parent) :
    ContainerWidget(ctx, parent),
    m_state(state)
{
}

Vector2 SizeLimit::desired_size(const Vector2 parent_size) const
{
   const auto child =
      m_content->desired_size(Vector2{std::min(parent_size.x, m_state.max_size.x), std::min(parent_size.y, m_state.max_size.y)});
   return {std::min(child.x, m_state.max_size.x), std::min(child.y, m_state.max_size.y)};
}

void SizeLimit::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_content->add_to_viewport(dimensions, cropping_mask);
}

void SizeLimit::remove_from_viewport()
{
   m_content->remove_from_viewport();
}

void SizeLimit::on_event(const Event& event)
{
   m_content->on_event(event);
}

}// namespace triglav::ui_core
