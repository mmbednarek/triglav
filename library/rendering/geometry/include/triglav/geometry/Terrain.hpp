#pragma once

#include "Geometry.hpp"

#include "triglav/Math.hpp"
#include "triglav/Name.hpp"
#include "triglav/io/Stream.hpp"

#include <optional>
#include <vector>

namespace triglav::geometry {

class Terrain
{
 public:
   struct MipLevel
   {
      std::vector<float> level;
      Vector2u dimensions;
      Vector2 fragment_dim;

      void init(const Vector2u& dim)
      {
         this->dimensions = dim;
         this->level.resize(dim.x * dim.y);
      }

      BoundingBox bounding_box(const Vector2u& offset) const
      {
         const auto ndc_min = 2.0f * (offset.cast<Vector2>() / dimensions.cast<Vector2>()) - Vector2{1};
         const auto ndc_max = 2.0f * ((offset.cast<Vector2>() + Vector2{1}) / dimensions.cast<Vector2>()) - Vector2{1};
         return BoundingBox{
            .min =
               {
                  ndc_min * Vector2{120.0f},
                  -0.01f,
               },
            .max =
               {
                  ndc_max * Vector2{120.0f},
                  level[offset.x + offset.y * this->dimensions.x] * 30.0f,
               },
         };
      }
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

   Terrain(Vector2u dimensions, TextureName default_texture);

   [[nodiscard]] Vector2u dimensions() const;
   [[nodiscard]] const std::vector<float>& heightmap() const;
   [[nodiscard]] const std::vector<Vector4b>& material_indices() const;
   [[nodiscard]] const std::vector<Vector4b>& blending() const;

   [[nodiscard]] std::vector<float>& mut_heightmap();
   [[nodiscard]] std::vector<Vector4b>& mut_blending();

   [[nodiscard]] RayHit ray_cast(const Ray& ray) const;

   bool serialize(io::IWriter& writer) const;
   static std::optional<Terrain> deserialize(io::IReader& reader);

 private:
   RayHit ray_cast_visit_level(u32 mip, const Vector2u& base, Ray& ray) const;
   void generate_mip_levels();

   Vector2u m_dimensions;
   Vector2 m_world_dimensions{240.0f, 240.0f};
   std::vector<TextureName> m_pallette;
   std::vector<float> m_heightmap;
   std::vector<MipLevel> m_heightmap_mips;
   std::vector<Vector4b> m_material_indices;
   std::vector<Vector4b> m_blending;
};

}// namespace triglav::geometry
