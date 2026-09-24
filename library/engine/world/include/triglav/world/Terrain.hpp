#pragma once

#include "triglav/io/Path.hpp"
#include "triglav/io/Stream.hpp"
#include "triglav/meta/Meta.hpp"

#include <vector>

namespace triglav::world {

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

   void save_to_file(const io::Path& path) const;

   bool serialize(io::IWriter& writer) const;
   static std::optional<Terrain> deserialize(io::IReader& reader);
   static std::optional<Terrain> load_from_file(const io::Path& path);

 private:
   Vector2u m_dimensions;
   std::vector<TextureName> m_pallette;
   std::vector<float> m_heightmap;
   std::vector<Vector4b> m_material_indices;
   std::vector<Vector4b> m_blending;
};

}// namespace triglav::world
