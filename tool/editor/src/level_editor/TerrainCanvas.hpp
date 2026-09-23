#pragma once

#include "triglav/Math.hpp"
#include "triglav/geometry/DebugMesh.hpp"
#include "triglav/world/World.hpp"

#include <optional>

namespace triglav::world {
class Level;
class Terrain;
}// namespace triglav::world

namespace triglav::editor {

class TerrainCanvas
{
 public:
   TerrainCanvas(world::Level& level, world::EntityID entity_id);

   void set_brush_size(float size);
   void shift(float amount, Vector2i coord) const;
   void level(float level, float strength, Vector2i coord) const;
   void smooth(float strength, Vector2i coord) const;
   void paint(float strength, Vector2i coord) const;

   [[nodiscard]] float sample(Vector2i coord) const;
   [[nodiscard]] std::optional<Vector3> trace_ray(const geometry::Ray& ray) const;
   [[nodiscard]] Vector2i world_pos_to_coord(Vector3 pos) const;
   [[nodiscard]] float height_to_world(float height) const;
   [[nodiscard]] float distance_to_world(float distance) const;
   [[nodiscard]] float brush_size() const;

 private:
   float sample_average(Vector2i coord) const;

   world::Level& m_level;
   world::EntityID m_entity_id;
   world::Terrain& m_terrain;
   float m_brush_size = 40.0f;
   i32 m_height_map_resolution = 1024;
   float m_world_size = 240.0f;
   float m_height_scale = 30.0f;
};

}// namespace triglav::editor