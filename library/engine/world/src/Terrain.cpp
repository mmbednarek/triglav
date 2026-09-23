#include "Terrain.hpp"

namespace triglav::world {

Terrain::Terrain(const Vector2u dimensions, const TextureName default_texture) :
    m_dimensions(dimensions)
{
   m_pallette.emplace_back(default_texture);

   const auto count = m_dimensions.x * m_dimensions.y;
   m_blending.resize(count, Vector4b{255, 0, 0, 0});
   m_heightmap.resize(count, 0);
   m_material_indices.resize(count, Vector4b{0, 0, 0, 0});
}

Vector2u Terrain::dimensions() const
{
   return m_dimensions;
}

const std::vector<float>& Terrain::heightmap() const
{
   return m_heightmap;
}

const std::vector<Vector4b>& Terrain::material_indices() const
{
   return m_material_indices;
}

const std::vector<Vector4b>& Terrain::blending() const
{
   return m_blending;
}

std::vector<float>& Terrain::mut_heightmap()
{
   return m_heightmap;
}

std::vector<Vector4b>& Terrain::mut_blending()
{
   return m_blending;
}

}// namespace triglav::world
