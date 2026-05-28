#include "TerrainShiftTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"

namespace triglav::editor {

TerrainShiftTool::TerrainShiftTool(LevelEditor& level_editor) :
    m_level_editor(level_editor),
    m_canvas(m_level_editor.scene())
{
}

bool TerrainShiftTool::on_use_start(const geometry::Ray& ray)
{
   if (ray.direction.z >= 0.0f || ray.origin.z <= 0.0f) {
      m_is_being_used = false;
      return true;
   }

   const float ray_length = -ray.origin.z / ray.direction.z;
   m_ground_position = Vector2{ray.origin + ray.direction * ray_length} / 120.0f;
   m_is_being_used = true;

   return true;
}

void TerrainShiftTool::on_mouse_moved(const Vector2 position)
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

void TerrainShiftTool::on_view_updated() {}

void TerrainShiftTool::on_use_end()
{
   m_is_being_used = false;
}

void TerrainShiftTool::on_left_tool()
{
   m_is_being_used = false;
}

void TerrainShiftTool::on_tick(const float delta_time)
{
   if (!m_is_being_used)
      return;

   const auto coord = Vector2i{1024.0f * 0.5f * (m_ground_position + Vector2{1.0f, 1.0f})};
   m_canvas.paint((m_is_downwards ? -0.2f : 0.2f) * delta_time, coord);
}

void TerrainShiftTool::on_modifiers(const desktop::ModifierFlags modifier_flags)
{
   m_is_downwards = modifier_flags & desktop::Modifier::Shift;
}

}// namespace triglav::editor
