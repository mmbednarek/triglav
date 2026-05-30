#pragma once

#include "TerrainTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class TerrainSmoothTool final : public TerrainTool
{
   TG_DEFINE_LOG_CATEGORY(TerrainSmoothTool)
 public:
   explicit TerrainSmoothTool(LevelEditor& level_editor);

   void on_tick(float delta_time) override;
};

}// namespace triglav::editor
