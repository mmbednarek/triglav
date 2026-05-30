#pragma once

#include "TerrainTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class LevelEditor;

class TerrainShiftTool final : public TerrainTool
{
   TG_DEFINE_LOG_CATEGORY(TerrainShiftTool)
 public:
   explicit TerrainShiftTool(LevelEditor& level_editor);

   void on_tick(float delta_time) override;
   void on_modifiers(desktop::ModifierFlags /*mods*/) override;

 private:
   bool m_is_downwards = false;
};

}// namespace triglav::editor
