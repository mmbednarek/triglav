#include "TerrainShiftTool.hpp"

#include "LevelEditor.hpp"

namespace triglav::editor {

TerrainShiftTool::TerrainShiftTool(LevelEditor& level_editor) :
    m_level_editor(level_editor)
{
}

bool TerrainShiftTool::on_use_start(const geometry::Ray& ray)
{
   if (ray.direction.z >= 0.0f || ray.origin.z <= 0.0f) {
      // if goes up ignore
      return true;
   }

   const float ray_length = -ray.origin.z / ray.direction.z;
   const auto ground_pos = Vector2{ray.origin + ray.direction * ray_length} / 120.0f;

   const auto uv = Vector2i{1024.0f * 0.5f * (ground_pos + Vector2{1.0f, 1.0f})};

   if (uv.x < 0 || uv.x > 1024 || uv.y < 0 || uv.y > 1024)
      return true;

   auto& terr = m_level_editor.scene().terrain();
   // const float curr_value = terr[uv.x + 1024 * uv.y];
   // const float target_value = curr_value + 0.1f;

   for (int y = -60; y <= 60; y++) {
      if (y + uv.y < 0)
         continue;

      for (int x = -60; x <= 60; x++) {
         if (x + uv.x < 0)
            continue;

         const float dx = static_cast<float>(x) / 60.0f;
         const float dy = static_cast<float>(y) / 60.0f;
         const float dist = std::sqrt(dx * dx + dy * dy);
         if (dist > 1.0f)
            continue;

         const float val = std::cos(dist * MATH_PI * 0.5f);
         // const auto v = terr[(x + uv.x) + 1024 * (y + uv.y)];

         // terr[(x + uv.x) + 1024 * (y + uv.y)] = std::lerp(target_value, v, val);
         terr[(x + uv.x) + 1024 * (y + uv.y)] += 0.1f * val;
      }
   }
   log_info("moving terrain at {} {} = {}", uv.x, uv.y, terr[(uv.x) + 1024 * (uv.y)]);

   m_level_editor.scene().publish_terrain_changes();

   return true;
}

void TerrainShiftTool::on_mouse_moved(Vector2 /*position*/) {}

void TerrainShiftTool::on_view_updated() {}

void TerrainShiftTool::on_use_end() {}

void TerrainShiftTool::on_left_tool() {}

}// namespace triglav::editor
