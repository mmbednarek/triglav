#include "widget/EmptySpace.hpp"

namespace triglav::ui_core {

EmptySpace::EmptySpace(Context& /*ctx*/, const State state) :
    m_state(state)
{
}

Vector2 EmptySpace::desired_size(Vector2 /*parentSize*/) const
{
   return m_state.size;
}

void EmptySpace::add_to_viewport(Vector4 /*dimensions*/) {}

void EmptySpace::remove_from_viewport() {}

}// namespace triglav::ui_core
