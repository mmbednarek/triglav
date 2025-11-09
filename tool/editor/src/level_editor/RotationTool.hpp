#pragma once

#include "ILevelEditorTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class LevelEditor;

class RotationTool final : public ILevelEditorTool
{
   TG_DEFINE_LOG_CATEGORY(RotationTool)
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
   Quaternion m_startingRotation{};
   bool m_isBeingUsed{false};
};

}// namespace triglav::editor