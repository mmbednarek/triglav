#include "Terrain.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/io/File.hpp"
#include "triglav/io/Serializer.hpp"

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

void Terrain::save_to_file(const io::Path& path) const
{
   auto file = io::open_file(path, io::FileMode::Create | io::FileMode::Write);
   assert(file.has_value());
   assert(asset::write_header(**file, ResourceType::Terrain));

   assert(this->serialize(**file));
}

bool Terrain::serialize(io::IWriter& writer) const
{
   io::Serializer serializer(writer);

   if (!serializer.write_u32(m_dimensions.x).has_value())
      return false;
   if (!serializer.write_u32(m_dimensions.y).has_value())
      return false;

   if (!serializer.write_u32(m_pallette.size()).has_value())
      return false;

   for (const auto& texture : m_pallette) {
      if (!serializer.write_name(texture.name()))
         return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(m_heightmap.data()), m_heightmap.size() * sizeof(float)}).has_value())
      return false;

   if (!writer.write({reinterpret_cast<const u8*>(m_blending.data()), m_blending.size() * sizeof(Vector4b)}).has_value())
      return false;

   if (!writer.write({reinterpret_cast<const u8*>(m_material_indices.data()), m_material_indices.size() * sizeof(Vector4b)}).has_value())
      return false;

   return true;
}

std::optional<Terrain> Terrain::deserialize(io::IReader& reader)
{
   io::Deserializer deserializer(reader);

   const auto x = deserializer.read_u32();
   const auto y = deserializer.read_u32();

   const auto palette_item_count = deserializer.read_u32();

   const TextureName first_texture{deserializer.read_name()};
   Terrain result{{x, y}, first_texture};

   for (u32 i = 1; i < palette_item_count; ++i) {
      result.m_pallette.emplace_back(deserializer.read_name());
   }

   const mem_size data_size = x * y;
   if (!reader.read({reinterpret_cast<u8*>(result.m_heightmap.data()), data_size * sizeof(float)}).has_value())
      return std::nullopt;
   if (!reader.read({reinterpret_cast<u8*>(result.m_blending.data()), data_size * sizeof(Vector4b)}).has_value())
      return std::nullopt;
   if (!reader.read({reinterpret_cast<u8*>(result.m_material_indices.data()), data_size * sizeof(Vector4b)}).has_value())
      return std::nullopt;

   return result;
}

std::optional<Terrain> Terrain::load_from_file(const io::Path& path)
{
   auto file = io::open_file(path, io::FileMode::Read);
   if (!file.has_value())
      return std::nullopt;

   auto header = asset::decode_header(**file);
   if (!header.has_value())
      return std::nullopt;

   return deserialize(**file);
}

}// namespace triglav::world
