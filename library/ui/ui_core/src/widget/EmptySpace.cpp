#include "widget/EmptySpace.hpp"

namespace triglav::ui_core {

EmptySpace::EmptySpace(Context& /*ctx*/, const State state, IWidget* /*parent*/) :
    m_state(state)
{
}

Vector2 EmptySpace::desired_size(Vector2 /*available_size*/) const
{
   return m_state.size;
}

void EmptySpace::add_to_viewport(Vector4 /*dimensions*/, const Vector4 /*cropping_mask*/) {}

void EmptySpace::remove_from_viewport() {}

}// namespace triglav::ui_core
