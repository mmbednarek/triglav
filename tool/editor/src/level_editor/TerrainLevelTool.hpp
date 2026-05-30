#pragma once

#include "TerrainTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class TerrainLevelTool final : public TerrainTool
{
   TG_DEFINE_LOG_CATEGORY(TerrainLevelTool)
 public:
   explicit TerrainLevelTool(LevelEditor& level_editor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_tick(float delta_time) override;

 private:
   float m_level = 0.0f;
};

}// namespace triglav::editor
