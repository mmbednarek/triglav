#include "TerrainTool.hpp"

#include "LevelEditor.hpp"

namespace triglav::editor {

TerrainTool::TerrainTool(LevelEditor& level_editor) :
    m_level_editor(level_editor),
    m_canvas(m_level_editor.scene())
{
}

bool TerrainTool::on_use_start(const geometry::Ray& ray)
{
   const auto pos = m_canvas.trace_ray(ray);
   if (!pos.has_value()) {
      m_is_being_used = false;
      return true;
   }
   m_is_being_used = true;
   m_pointer_position = *pos;
   return true;
}

void TerrainTool::on_mouse_moved(const Vector2 position)
{
   if (!m_is_being_used)
      return;

   const auto ray = m_level_editor.viewport_ray(position);
   const auto pos = m_canvas.trace_ray(ray);
   if (!pos.has_value()) {
      m_is_being_used = false;
   }

   m_pointer_position = *pos;
}

void TerrainTool::on_view_updated() {}

void TerrainTool::on_use_end()
{
   m_is_being_used = false;
}

void TerrainTool::on_left_tool()
{
   m_is_being_used = false;
}

}// namespace triglav::editor
