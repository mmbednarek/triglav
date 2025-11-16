#pragma once

#include "ILevelEditorTool.hpp"

namespace triglav::editor {

class LevelEditor;

class TranslationTool final : public ILevelEditorTool
{
 public:
   explicit TranslationTool(LevelEditor& levelEditor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;

 private:
   LevelEditor& m_levelEditor;

   std::optional<Axis> m_transformAxis;
   geometry::BoundingBox m_arrow_x_bb{};
   geometry::BoundingBox m_arrow_y_bb{};
   geometry::BoundingBox m_arrow_z_bb{};

   Transform3D m_startingTransform;
   Vector3 m_startingHit{};
};

}// namespace triglav::editor
