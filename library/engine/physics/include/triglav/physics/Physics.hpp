#pragma once

#include "triglav/Math.hpp"
#include "triglav/geometry/Geometry.hpp"
#include "triglav/meta/Meta.hpp"

namespace triglav::physics {

struct Movement
{
   TG_META_STRUCT_BODY(Movement)

   Vector3 motion{};
   bool is_on_ground{false};
};

struct CapsuleComponent
{
   TG_META_STRUCT_BODY(CapsuleComponent)

   geometry::Capsule capsule;
};

}// namespace triglav::physics
