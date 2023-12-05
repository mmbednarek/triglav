#pragma once

#include "Core.h"
#include "geometry/Mesh.h"

#include <string_view>

namespace renderer {

Mesh create_sphere(int segment_count, int ring_count, float radius);
Mesh create_cilinder(int segment_count, int ring_count, float radius, float depth);
Mesh create_inner_box(float width, float height, float depth);
Mesh from_mesh(object_reader::Mesh& mesh);

}