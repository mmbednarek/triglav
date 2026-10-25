#pragma once

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/io/Stream.hpp"

#include <optional>
#include <vector>

namespace triglav::geometry {

class Terrain
{
 public:
   Terrain(Vector2u dimensions, TextureName default_texture);

   [[nodiscard]] Vector2u dimensions() const;
   [[nodiscard]] const std::vector<float>& heightmap() const;
   [[nodiscard]] const std::vector<Vector4b>& material_indices() const;
   [[nodiscard]] const std::vector<Vector4b>& blending() const;

   [[nodiscard]] std::vector<float>& mut_heightmap();
   [[nodiscard]] std::vector<Vector4b>& mut_blending();

   bool serialize(io::IWriter& writer) const;
   static std::optional<Terrain> deserialize(io::IReader& reader);

 private:
   Vector2u m_dimensions;
   std::vector<TextureName> m_pallette;
   std::vector<float> m_heightmap;
   std::vector<Vector4b> m_material_indices;
   std::vector<Vector4b> m_blending;
};

}// namespace triglav::geometry
