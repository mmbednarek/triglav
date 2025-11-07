#pragma once

#include "ILevelEditorTool.hpp"

namespace triglav::editor {

class LevelEditor;

class RotationTool : public ILevelEditorTool
{
 public:
   explicit RotationTool(LevelEditor& levelEditor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;

 private:
   LevelEditor& m_levelEditor;

   std::optional<Axis> m_rotationAxis;
   geometry::BoundingBox m_rotator_x_bb{};
   geometry::BoundingBox m_rotator_y_bb{};
   geometry::BoundingBox m_rotator_z_bb{};
   float m_baseAngle{};
   float m_oldAngle{};
   bool m_isBeingUsed{false};

   Vector3 m_translationOffset{};
};

}// namespace triglav::editor