#pragma once

#include "triglav/Math.hpp"
#include "triglav/meta/Meta.hpp"

namespace triglav::physics {

struct Movement
{
   TG_META_STRUCT_BODY(Movement)

   Vector3 motion{};
   bool is_on_ground{false};
};

}// namespace triglav::physics
