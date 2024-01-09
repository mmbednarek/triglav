#pragma once

#include "Mesh.h"

namespace geometry {

Mesh create_box(const Extent3D & extent);
Mesh create_sphere(int segment_count, int ring_count, float radius);
// Mesh create_cilinder(int segment_count, int ring_count, float radius, float depth);

}