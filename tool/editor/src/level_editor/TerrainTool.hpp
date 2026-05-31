#pragma once

#include "ILevelEditorTool.hpp"
#include "TerrainCanvas.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class LevelEditor;

class TerrainTool : public ILevelEditorTool
{
 public:
   explicit TerrainTool(LevelEditor& level_editor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;
   void set_brush_size(float size);
   void set_strength(float size);

 protected:
   LevelEditor& m_level_editor;
   TerrainCanvas m_canvas;
   bool m_is_being_used = false;
   Vector2i m_pointer_position{};
   float m_strength{1.0f};
};

}// namespace triglav::editor
