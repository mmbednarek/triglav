#include "Terrain.hpp"

#include "triglav/Math.hpp"
#include "triglav/io/Serializer.hpp"

namespace triglav::geometry {

namespace {

Vector3 calculate_normal(const Terrain& terrain, const Vector2 uv)
{
   const auto d = 2.0f * terrain.frag_uv();
   const float hl = terrain.sample_height(uv + Vector2(-d.x, 0));
   const float hr = terrain.sample_height(uv + Vector2(d.x, 0));
   const float hb = terrain.sample_height(uv + Vector2(0, -d.y));
   const float hf = terrain.sample_height(uv + Vector2(0, d.y));

   const Vector3 tangent = Vector3(d.x * 240.0, 0.0, (hr - hl) * 30.0f).normalize();
   const Vector3 bitangent = Vector3(0.0, d.y * 240.0, (hf - hb) * 30.0f).normalize();

   return tangent.cross(bitangent).normalize();
}

}// namespace

void Terrain::MipLevel::init(const Vector2u& dim)
{
   this->dimensions = dim;
   this->level.resize(dim.x * dim.y);
}

void Terrain::MipLevel::init_from_parent(const MipLevel& parent)
{
   this->dimensions = {parent.dimensions.x >> 1, parent.dimensions.y >> 1};
   this->level.resize(this->dimensions.x * this->dimensions.y);

   for (u32 y = 0; y < this->dimensions.y; ++y) {
      for (u32 x = 0; x < this->dimensions.x; ++x) {
         std::array parent_levels{
            parent[2 * x + 0, 2 * y + 0],
            parent[2 * x + 1, 2 * y + 0],
            parent[2 * x + 0, 2 * y + 1],
            parent[2 * x + 1, 2 * y + 1],
         };

         this->get({x, y}) = *std::ranges::max_element(parent_levels);
      }
   }
}

BoundingBox Terrain::MipLevel::bounding_box(const Terrain& terrain, const Vector2u& offset) const
{
   const auto ndc_min = 2.0f * (offset.cast<Vector2>() / dimensions.cast<Vector2>()) - Vector2{1};
   const auto ndc_max = 2.0f * ((offset.cast<Vector2>() + Vector2{1}) / dimensions.cast<Vector2>()) - Vector2{1};
   return BoundingBox{
      .min =
         Vector3{
            ndc_min * terrain.m_half_world_size.xy(),
            0.0f,
         },
      .max =
         Vector3{
            ndc_max * terrain.m_half_world_size.xy(),
            this->get(offset) * terrain.m_half_world_size.z,
         },
   };
}

Terrain::Terrain(const Vector2u dimensions, const TextureName default_texture)
{
   const u32 mip_count = calculate_mip_count(dimensions) - 1;// Last mip should be 4x4
   if (mip_count < 1)
      return;
   m_heightmap_mips.resize(mip_count);
   m_heightmap_mips[0].init({dimensions.x, dimensions.y});

   m_pallette.emplace_back(default_texture);

   const auto count = dimensions.x * dimensions.y;
   m_blending.resize(count, Vector4b{255, 0, 0, 0});
   m_material_indices.resize(count, Vector4b{0, 0, 0, 0});
}

Vector2u Terrain::dimensions() const
{
   return m_heightmap_mips[0].dimensions;
}

const std::vector<float>& Terrain::heightmap() const
{
   return m_heightmap_mips[0].level;
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
   return m_heightmap_mips[0].level;
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

Terrain::SweepHit Terrain::sweep_cast(const BoundingBox& dynamic_box, const Vector3& movement) const
{
   float best_score = INF;
   return this->sweep_cast_visit_level(m_heightmap_mips.size() - 1, {0, 0}, dynamic_box, movement, best_score);
}

BoundingBox Terrain::bounding_box() const
{
   return {
      {-m_half_world_size.xy(), 0.0f},
      m_half_world_size,
   };
}

geometry::SweepHit Terrain::generic_sweep_cast(const BoundingBox& box, const Vector3& movement)
{
   const auto res = this->sweep_cast(box, movement);
   return {res.time, res.normal};
}

float Terrain::generic_ray_cast(const Ray& ray)
{
   const auto res = this->ray_cast(ray);
   return res.distance;
}

float Terrain::sample_height(const Vector2& uv) const
{
   const auto coord = uv * m_heightmap_mips[0].dimensions.cast<Vector2>();
   const auto floor = coord.floor().cast<Vector2u>();
   const auto ceil = coord.ceil().cast<Vector2u>();

   std::array heights{
      m_heightmap_mips[0].get(floor),
      m_heightmap_mips[0].get({floor.x, ceil.y}),
      m_heightmap_mips[0].get({ceil.x, floor.y}),
      m_heightmap_mips[0].get(ceil),
   };
   const auto fmod = coord.fmod(1.0f);

   return lerp(lerp(heights[0], heights[2], fmod.x), lerp(heights[1], heights[3], fmod.x), fmod.y);
}

Vector2 Terrain::frag_uv() const
{
   return m_heightmap_mips[0].dimensions.cast<Vector2>().reciprocal();
}

bool Terrain::serialize(io::IWriter& writer) const
{
   io::Serializer serializer(writer);

   if (!serializer.write_u32(m_heightmap_mips[0].dimensions.x).has_value())
      return false;
   if (!serializer.write_u32(m_heightmap_mips[0].dimensions.y).has_value())
      return false;

   if (!serializer.write_u32(m_pallette.size()).has_value())
      return false;

   for (const auto& texture : m_pallette) {
      if (!serializer.write_name(texture.name()))
         return false;
   }

   if (!writer.write({reinterpret_cast<const u8*>(m_heightmap_mips[0].level.data()), m_heightmap_mips[0].level.size() * sizeof(float)})
           .has_value())
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
   if (!reader.read({reinterpret_cast<u8*>(result.m_heightmap_mips[0].level.data()), data_size * sizeof(float)}).has_value())
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
   std::ranges::transform(offsets, bounding_boxes.begin(),
                          [&](const Vector2u& offset) { return level.bounding_box(*this, 2u * base + offset); });

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
            .coordinate = base * 2u + offsets[index],
            .distance = hit.min_distance,
         };
      }

      const auto result = this->ray_cast_visit_level(mip - 1, base * 2u + offsets[index], ray);
      if (result) {
         best_result = result;
      }
   }

   return best_result;
}

Terrain::SweepHit Terrain::sweep_cast_visit_level(const u32 mip, const Vector2u& base, const BoundingBox& dynamic_box,
                                                  const Vector3& movement, float& best_score) const
{
   const auto& level = m_heightmap_mips[mip];

   constexpr static std::array<Vector2u, 4> offsets{
      Vector2u{0, 0},
      Vector2u{1, 0},
      Vector2u{0, 1},
      Vector2u{1, 1},
   };

   std::array<BoundingBox, 4> bounding_boxes{};
   std::ranges::transform(offsets, bounding_boxes.begin(),
                          [&](const Vector2u& offset) { return level.bounding_box(*this, 2u * base + offset); });

   std::array<std::pair<u32, BoundingBox::SweepResult>, 4> hits{};
   std::ranges::transform(bounding_boxes, hits.begin(), [&, i = 0u](const BoundingBox& bb) mutable {
      return std::make_pair(i++, bb.intersect_swept(dynamic_box, movement));
   });

   std::ranges::sort(hits, [](const std::pair<u32, BoundingBox::SweepResult>& lhs, const std::pair<u32, BoundingBox::SweepResult>& rhs) {
      return lhs.second.entry_time < rhs.second.entry_time;
   });

   auto best_result = SweepHit::miss();

   for (const auto& [index, hit] : hits) {
      if (!hit.has_hit())
         break;
      if (hit.entry_time > best_score)
         break;

      if (mip == 0) {
         best_score = hit.entry_time;
         const auto coord = base * 2u + offsets[index];
         const auto uv = coord.cast<Vector2>() / m_heightmap_mips[0].dimensions.cast<Vector2>();
         return SweepHit{
            .time = hit.entry_time,
            .normal = calculate_normal(*this, uv),
            .coordinate = base * 2u + offsets[index],
         };
      }

      const auto sweep_result = this->sweep_cast_visit_level(mip - 1, base * 2u + offsets[index], dynamic_box, movement, best_score);
      if (sweep_result) {
         best_result = sweep_result;
      }
   }
   return best_result;
}

void Terrain::generate_mip_levels()
{
   for (u32 mip_level = 1; mip_level < m_heightmap_mips.size(); ++mip_level) {
      m_heightmap_mips[mip_level].init_from_parent(m_heightmap_mips[mip_level - 1]);
   }
}

}// namespace triglav::geometry
