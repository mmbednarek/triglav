#include "TerrainPaintTool.hpp"

#include "LevelEditor.hpp"

namespace triglav::editor {

TerrainPaintTool::TerrainPaintTool(LevelEditor& level_editor) :
    TerrainTool(level_editor)
{
}

void TerrainPaintTool::on_tick(const float delta_time)
{
   if (!m_is_being_used)
      return;

   m_canvas.paint(m_strength * delta_time, m_pointer_position);
}

}// namespace triglav::editor
