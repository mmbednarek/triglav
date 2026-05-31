#include "TerrainLevelTool.hpp"

#include "LevelEditor.hpp"

namespace triglav::editor {

TerrainLevelTool::TerrainLevelTool(LevelEditor& level_editor) :
    TerrainTool(level_editor)
{
}

bool TerrainLevelTool::on_use_start(const geometry::Ray& ray)
{
   const auto status = TerrainTool::on_use_start(ray);
   m_level = m_canvas.sample(m_pointer_position);
   return status;
}

void TerrainLevelTool::on_tick(const float delta_time)
{
   if (!m_is_being_used)
      return;

   m_canvas.level(m_level, m_strength * delta_time, m_pointer_position);
}

}// namespace triglav::editor
