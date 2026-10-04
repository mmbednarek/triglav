#pragma once

#include "triglav/geometry/Geometry.hpp"

namespace triglav::render_objects {

struct UniformBufferObject
{
   alignas(16) Matrix4x4 model;
   alignas(16) Matrix4x4 view;
   alignas(16) Matrix4x4 proj;
   alignas(16) Matrix4x4 normal;
   alignas(4) Vector3 view_pos;
};

struct SpriteUBO
{
   // 3x3 matrix needs to aligned by 4 floats
   // so we use 4x4 instead.
   Matrix4x4 transform;
};

struct ShadowMapUBO
{
   alignas(16) Matrix4x4 mvp;
};

}// namespace triglav::render_objects