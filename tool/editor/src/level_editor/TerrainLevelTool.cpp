#include "TerrainLevelTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"

namespace triglav::editor {

TerrainLevelTool::TerrainLevelTool(LevelEditor& level_editor) :
    m_level_editor(level_editor),
    m_canvas(m_level_editor.scene())
{
}

bool TerrainLevelTool::on_use_start(const geometry::Ray& ray)
{
   if (ray.direction.z >= 0.0f || ray.origin.z <= 0.0f) {
      m_is_being_used = false;
      return true;
   }

   const float ray_length = -ray.origin.z / ray.direction.z;
   m_ground_position = Vector2{ray.origin + ray.direction * ray_length} / 120.0f;
   m_is_being_used = true;

   const auto coord = Vector2i{1024.0f * 0.5f * (m_ground_position + Vector2{1.0f, 1.0f})};
   m_level = m_canvas.sample(coord);

   return true;
}

void TerrainLevelTool::on_mouse_moved(const Vector2 position)
{
   if (!m_is_being_used)
      return;

   const auto ray = m_level_editor.viewport_ray(position);
   if (ray.direction.z >= 0.0f || ray.origin.z <= 0.0f) {
      m_is_being_used = false;
      return;
   }

   const float ray_length = -ray.origin.z / ray.direction.z;
   m_ground_position = Vector2{ray.origin + ray.direction * ray_length} / 120.0f;
}

void TerrainLevelTool::on_view_updated() {}

void TerrainLevelTool::on_use_end()
{
   m_is_being_used = false;
}

void TerrainLevelTool::on_left_tool()
{
   m_is_being_used = false;
}

void TerrainLevelTool::on_tick(const float delta_time)
{
   if (!m_is_being_used)
      return;

   const auto coord = Vector2i{1024.0f * 0.5f * (m_ground_position + Vector2{1.0f, 1.0f})};
   m_canvas.level(m_level, 2.0f * delta_time, coord);
}

}// namespace triglav::editor
