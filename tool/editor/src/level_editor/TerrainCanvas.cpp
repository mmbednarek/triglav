#include "TerrainCanvas.hpp"

#include "triglav/renderer/Scene.hpp"

namespace triglav::editor {

TerrainCanvas::TerrainCanvas(renderer::Scene& scene) :
    m_scene(scene)
{
}

void TerrainCanvas::set_brush_size(const float size)
{
   m_brush_size = size;
}

void TerrainCanvas::paint(const float amount, const Vector2i coord) const
{
   if (coord.x < 0 || coord.x > 1024 || coord.y < 0 || coord.y > 1024)
      return;

   auto& terr = m_scene.terrain();

   const i32 brush_size_int = static_cast<i32>(m_brush_size);
   for (i32 y = -brush_size_int; y <= brush_size_int; y++) {
      if (y + coord.y < 0)
         continue;

      for (i32 x = -brush_size_int; x <= brush_size_int; x++) {
         if (x + coord.x < 0)
            continue;

         const float dx = static_cast<float>(x) / m_brush_size;
         const float dy = static_cast<float>(y) / m_brush_size;
         const float dist = std::sqrt(dx * dx + dy * dy);
         if (dist > 1.0f)
            continue;

         const float shape = std::sin(dist * MATH_PI * 0.5f);
         const auto index = (x + coord.x) + 1024 * (coord.y + y);
         terr[index] += amount * (1.0f - shape);
      }
   }

   m_scene.publish_terrain_changes();
}

void TerrainCanvas::level(const float level, const float strength, const Vector2i coord) const
{
   if (coord.x < 0 || coord.x > 1024 || coord.y < 0 || coord.y > 1024)
      return;

   auto& terr = m_scene.terrain();

   const i32 brush_size_int = static_cast<i32>(m_brush_size);
   for (i32 y = -brush_size_int; y <= brush_size_int; y++) {
      if (y + coord.y < 0)
         continue;

      for (i32 x = -brush_size_int; x <= brush_size_int; x++) {
         if (x + coord.x < 0)
            continue;

         const float dx = static_cast<float>(x) / m_brush_size;
         const float dy = static_cast<float>(y) / m_brush_size;
         const float dist = std::sqrt(dx * dx + dy * dy);
         if (dist > 1.0f)
            continue;
         const float shape = std::cos(dist * MATH_PI * 0.5f);
         const auto index = (x + coord.x) + 1024 * (coord.y + y);
         terr[index] = std::lerp(terr[index], level, shape * strength);
      }
   }

   m_scene.publish_terrain_changes();
}

void TerrainCanvas::smooth(const float strength, const Vector2i coord) const
{
   if (coord.x < 0 || coord.x > 1024 || coord.y < 0 || coord.y > 1024)
      return;

   const auto level = this->sample_average(coord);

   auto& terr = m_scene.terrain();

   const i32 brush_size_int = static_cast<i32>(m_brush_size);
   for (i32 y = -brush_size_int; y <= brush_size_int; y++) {
      if (y + coord.y < 0)
         continue;

      for (i32 x = -brush_size_int; x <= brush_size_int; x++) {
         if (x + coord.x < 0)
            continue;

         const float dx = static_cast<float>(x) / m_brush_size;
         const float dy = static_cast<float>(y) / m_brush_size;
         const float dist = std::sqrt(dx * dx + dy * dy);
         if (dist > 1.0f)
            continue;
         const float shape = std::cos(dist * MATH_PI * 0.5f);
         const auto index = (x + coord.x) + 1024 * (coord.y + y);
         terr[index] = std::lerp(terr[index], level, shape * strength);
      }
   }

   m_scene.publish_terrain_changes();
}

float TerrainCanvas::sample(const Vector2i coord) const
{
   if (coord.x < 0 || coord.x > 1024 || coord.y < 0 || coord.y > 1024)
      return 0.0f;

   auto& terr = m_scene.terrain();
   return terr[coord.x + 1024 * coord.y];
}

std::optional<Vector2i> TerrainCanvas::trace_ray(const geometry::Ray& ray) const
{
   if (ray.direction.z >= 0.0f || ray.origin.z <= 0.0f) {
      return std::nullopt;
   }

   // TODO: Update to hi-z approach
   const float ray_length = -ray.origin.z / ray.direction.z;
   const auto ground_position = Vector2{ray.origin + ray.direction * ray_length} / 120.0f;
   const auto coord = Vector2i{1024.0f * 0.5f * (ground_position + Vector2{1.0f, 1.0f})};

   if (coord.x < 0 || coord.x > 1024 || coord.y < 0 || coord.y > 1024)
      return std::nullopt;

   return coord;
}

float TerrainCanvas::sample_average(const Vector2i coord) const
{
   auto& terr = m_scene.terrain();

   const i32 brush_size_int = static_cast<i32>(m_brush_size);

   float sum = 0.0f;
   i32 count = 0;

   for (i32 y = -brush_size_int; y <= brush_size_int; y++) {
      if (y + coord.y < 0)
         continue;

      for (i32 x = -brush_size_int; x <= brush_size_int; x++) {
         if (x + coord.x < 0)
            continue;

         const auto index = (x + coord.x) + 1024 * (coord.y + y);
         sum += terr[index];
         ++count;
      }
   }


   return sum / static_cast<float>(count);
}

}// namespace triglav::editor
