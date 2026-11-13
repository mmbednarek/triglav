#pragma once

#include "Geometry.hpp"

namespace triglav::geometry
{

SweepHit static_sweep_cast(const TraversableHeader* static_object, const TraversableHeader* dynamic_object, const Vector3& movement_vec);
SweepHit dynamic_sweep_cast(const TraversableHeader* lhs, const Vector3& lhs_movement, const TraversableHeader* rhs, const Vector3& rhs_movement);

}