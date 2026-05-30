#pragma once

#include "triglav/Math.hpp"
#include "triglav/geometry/DebugMesh.hpp"

#include <optional>

namespace triglav::renderer {
class Scene;
}

namespace triglav::editor {

class TerrainCanvas
{
 public:
   explicit TerrainCanvas(renderer::Scene& scene);

   void set_brush_size(float size);
   void paint(float amount, Vector2i coord) const;
   void level(float level, float strength, Vector2i coord) const;
   void smooth(float strength, Vector2i coord) const;
   float sample(Vector2i coord) const;
   std::optional<Vector2i> trace_ray(const geometry::Ray& ray) const;

 private:
   float sample_average(Vector2i coord) const;

   renderer::Scene& m_scene;
   float m_brush_size = 40.0f;
};

}// namespace triglav::editor