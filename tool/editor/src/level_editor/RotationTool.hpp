#pragma once

#include "ILevelEditorTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class LevelEditor;

class RotationTool final : public ILevelEditorTool
{
   TG_DEFINE_LOG_CATEGORY(RotationTool)
 public:
   explicit RotationTool(LevelEditor& level_editor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;

 private:
   LevelEditor& m_level_editor;

   std::optional<Axis> m_rotation_axis;
   geometry::BoundingBox m_rotator_x_bb{};
   geometry::BoundingBox m_rotator_y_bb{};
   geometry::BoundingBox m_rotator_z_bb{};
   float m_base_angle{};
   Transform3D m_starting_transform;
   // Vector3 m_starting_translation{};
   // Quaternion m_starting_rotation{};
   bool m_is_being_used{false};
};

}// namespace triglav::editor