#pragma once

#include "TerrainTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class TerrainPaintTool final : public TerrainTool
{
   TG_DEFINE_LOG_CATEGORY(TerrainPaintTool)
 public:
   explicit TerrainPaintTool(LevelEditor& level_editor);

   void on_tick(float delta_time) override;
};

}// namespace triglav::editor
