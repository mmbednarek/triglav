#include "MeshData.hpp"

namespace triglav::geometry {

Vector3 MeshData::vertex_position_at(const u32 index)
{
   return this->vertex_data.vertex_buffer.get_location(index, this->vertex_data.index_buffer.at(index));
}

u32 MeshData::primitive_count() const
{
   return this->vertex_data.index_buffer.size();
}

}// namespace triglav::geometry
