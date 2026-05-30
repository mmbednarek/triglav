#include "TerrainShiftTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"

namespace triglav::editor {

TerrainShiftTool::TerrainShiftTool(LevelEditor& level_editor) :
    TerrainTool(level_editor)
{
}

void TerrainShiftTool::on_tick(const float delta_time)
{
   if (!m_is_being_used)
      return;

   m_canvas.paint((m_is_downwards ? -0.2f : 0.2f) * delta_time, m_pointer_position);
}

void TerrainShiftTool::on_modifiers(const desktop::ModifierFlags modifier_flags)
{
   m_is_downwards = modifier_flags & desktop::Modifier::Shift;
}

}// namespace triglav::editor
