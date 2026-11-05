#include "Terrain.hpp"

#include "triglav/Math.hpp"
#include "triglav/io/Serializer.hpp"

namespace triglav::geometry {

namespace {

void generate_mip_level(const Vector2u& parent_dims, const std::vector<float>& parent, const Vector2u& child_dims,
                        std::vector<float>& child)
{
   for (u32 y = 0; y < child_dims.y; ++y) {
      for (u32 x = 0; x < child_dims.x; ++x) {
         std::array parent_levels{
            parent[(2 * x + 0) + (2 * y + 0) * parent_dims.x],
            parent[(2 * x + 1) + (2 * y + 0) * parent_dims.x],
            parent[(2 * x + 0) + (2 * y + 1) * parent_dims.x],
            parent[(2 * x + 1) + (2 * y + 1) * parent_dims.x],
         };

         child[x + y * child_dims.x] = *std::ranges::max_element(parent_levels);
      }
   }
}

}// namespace

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

Terrain::RayHit Terrain::ray_cast(const Ray& ray) const
{
   Ray mut_ray = ray;
   return this->ray_cast_visit_level(m_heightmap_mips.size() - 1, {0, 0}, mut_ray);
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

   result.generate_mip_levels();

   return result;
}

Terrain::RayHit Terrain::ray_cast_visit_level(const u32 mip, const Vector2u& base, Ray& ray) const
{
   const auto& level = m_heightmap_mips[mip];

   constexpr static std::array<Vector2u, 4> offsets{
      Vector2u{0, 0},
      Vector2u{1, 0},
      Vector2u{0, 1},
      Vector2u{1, 1},
   };

   std::array<BoundingBox, 4> bounding_boxes{};
   std::ranges::transform(offsets, bounding_boxes.begin(), [&](const Vector2u& offset) { return level.bounding_box(2u * base + offset); });

   std::array<std::pair<u32, BoundingBox::RayHit>, 4> hits{};
   std::ranges::transform(bounding_boxes, hits.begin(),
                          [&, i = 0u](const BoundingBox& bb) mutable { return std::make_pair(i++, bb.intersect(ray)); });

   std::ranges::sort(hits, [](const std::pair<u32, BoundingBox::RayHit>& lhs, const std::pair<u32, BoundingBox::RayHit>& rhs) {
      return lhs.second.min_distance < rhs.second.min_distance;
   });

   auto best_result = RayHit::miss();

   for (const auto& [index, hit] : hits) {
      if (!hit.has_hit())
         break;
      if (hit.min_distance > ray.distance)
         break;

      if (mip == 0) {
         ray.distance = hit.min_distance;
         return RayHit{
            .coordinate = (base * 2u + offsets[index]) * 2u,
            .distance = hit.min_distance,
         };
      }

      best_result = this->ray_cast_visit_level(mip - 1, base * 2u + offsets[index], ray);
   }

   return best_result;
}

void Terrain::generate_mip_levels()
{
   const u32 mip_count = calculate_mip_count(m_dimensions) - 2;
   if (mip_count < 1)
      return;
   m_heightmap_mips.resize(mip_count);

   m_heightmap_mips[0].init({m_dimensions.x >> 1, m_dimensions.y >> 1});

   generate_mip_level(m_dimensions, m_heightmap, m_heightmap_mips[0].dimensions, m_heightmap_mips[0].level);

   for (u32 mip_level = 1; mip_level < mip_count; ++mip_level) {
      const auto& parent = m_heightmap_mips[mip_level - 1];

      auto& child = m_heightmap_mips[mip_level];
      child.init({parent.dimensions.x >> 1, parent.dimensions.y >> 1});
      child.fragment_dim = {m_world_dimensions.x / child.dimensions.x, m_world_dimensions.y / child.dimensions.y};

      generate_mip_level(parent.dimensions, parent.level, child.dimensions, child.level);
   }
}

}// namespace triglav::geometry
