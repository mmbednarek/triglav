#include "TerrainSmoothTool.hpp"

#include "LevelEditor.hpp"

namespace triglav::editor {

TerrainSmoothTool::TerrainSmoothTool(LevelEditor& level_editor) :
   TerrainTool(level_editor)
{
}

void TerrainSmoothTool::on_tick(const float delta_time)
{
   if (!m_is_being_used)
      return;

   m_canvas.smooth(delta_time, m_pointer_position);
}

}// namespace triglav::editor
