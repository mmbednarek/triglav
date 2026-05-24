#pragma once

#include "ILevelEditorTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class LevelEditor;

class TerrainShiftTool final : public ILevelEditorTool
{
   TG_DEFINE_LOG_CATEGORY(TerrainShiftTool)
 public:
   explicit TerrainShiftTool(LevelEditor& level_editor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;

 private:
   LevelEditor& m_level_editor;
};

}// namespace triglav::editor
