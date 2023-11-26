#pragma once

#include "Core.h"

namespace renderer {

Mesh create_sphere(int segment_count, int ring_count, float radius);
Mesh create_cilinder(int segment_count, int ring_count, float radius, float depth);
Mesh create_inner_box(float width, float height, float depth);
Mesh from_object(const MeshObject& object);

}