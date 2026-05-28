#pragma once

#include "triglav/Math.hpp"

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
   float sample(Vector2i coord) const;

 private:
   renderer::Scene& m_scene;
   float m_brush_size = 40.0f;
};

}// namespace triglav::editor