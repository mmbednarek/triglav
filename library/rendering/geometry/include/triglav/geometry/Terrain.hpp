#pragma once

#include "Geometry.hpp"

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/io/Stream.hpp"

#include <optional>
#include <vector>

namespace triglav::geometry {

class Terrain : public ITraversable
{
 public:
   struct MipLevel
   {
      std::vector<float> level;
      Vector2u dimensions;

      void init(const Vector2u& dim);
      void init_from_parent(const MipLevel& parent);

      [[nodiscard]] float& get(const Vector2u& coord)
      {
         return this->level[coord.x + coord.y * this->dimensions.x];
      }

      [[nodiscard]] float get(const Vector2u& coord) const
      {
         return this->level[coord.x + coord.y * this->dimensions.x];
      }

      [[nodiscard]] float operator[](const u32 x, const u32 y) const
      {
         return this->get({x, y});
      }

      [[nodiscard]] BoundingBox bounding_box(const Terrain& terrain, const Vector2u& offset) const;
   };

   struct RayHit
   {
      Vector2u coordinate{};
      float distance{};

      [[nodiscard]] explicit operator bool() const
      {
         return distance < INF;
      }

      static RayHit miss()
      {
         return {
            .coordinate = {},
            .distance = INF,
         };
      }
   };

   struct SweepHit
   {
      float time{INF};
      Vector3 normal{};
      Vector2u coordinate{};

      [[nodiscard]] explicit operator bool() const
      {
         return time < INF;
      }

      static SweepHit miss()
      {
         return SweepHit{};
      }

      [[nodiscard]] float score() const
      {
         return time;
      }
   };

   Terrain(Vector2u dimensions, TextureName default_texture);

   [[nodiscard]] Vector2u dimensions() const;
   [[nodiscard]] const std::vector<float>& heightmap() const;
   [[nodiscard]] const std::vector<Vector4b>& material_indices() const;
   [[nodiscard]] const std::vector<Vector4b>& blending() const;

   [[nodiscard]] std::vector<float>& mut_heightmap();
   [[nodiscard]] std::vector<Vector4b>& mut_blending();

   [[nodiscard]] RayHit ray_cast(const Ray& ray) const;
   [[nodiscard]] SweepHit sweep_cast(const BoundingBox& dynamic_box, const Vector3& movement) const;
   [[nodiscard]] BoundingBox bounding_box() const;

   [[nodiscard]] geometry::SweepHit generic_sweep_cast(const BoundingBox& box, const Vector3& movement) override;
   [[nodiscard]] float generic_ray_cast(const Ray& ray) override;

   [[nodiscard]] float sample_height(const Vector2& uv) const;
   [[nodiscard]] Vector2 frag_uv() const;

   bool serialize(io::IWriter& writer) const;
   static std::optional<Terrain> deserialize(io::IReader& reader);

 private:
   RayHit ray_cast_visit_level(u32 mip, const Vector2u& base, Ray& ray) const;
   SweepHit sweep_cast_visit_level(u32 mip, const Vector2u& base, const BoundingBox& dynamic_box, const Vector3& movement,
                                   float& best_score) const;
   void generate_mip_levels();

   Vector3 m_half_world_size{120.0f, 120.0f, 30.0f};
   std::vector<TextureName> m_pallette;
   std::vector<MipLevel> m_heightmap_mips;
   std::vector<Vector4b> m_material_indices;
   std::vector<Vector4b> m_blending;
};

}// namespace triglav::geometry
