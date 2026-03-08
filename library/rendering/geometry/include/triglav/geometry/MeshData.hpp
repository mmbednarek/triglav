#pragma once

#include "Geometry.hpp"
#include "VertexBuffer.hpp"

namespace triglav::geometry {

struct DeviceMesh
{
   graphics_api::Buffer vertex_buffer;
   graphics_api::IndexArray index_buffer;
   std::vector<VertexGroup> ranges;
};

struct VertexData
{
   VertexBuffer vertex_buffer;
   std::vector<u32> index_buffer;
};

struct MeshData
{
   VertexData vertex_data;
   BoundingBox bounding_box;
};

}// namespace triglav::geometry
