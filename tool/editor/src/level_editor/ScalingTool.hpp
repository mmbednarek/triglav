#pragma once

#include "ILevelEditorTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class LevelEditor;

class ScalingTool final : public ILevelEditorTool
{
   TG_DEFINE_LOG_CATEGORY(ScalingTool)
 public:
   explicit ScalingTool(LevelEditor& level_editor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;

 private:
   std::optional<Axis> get_transform_axis(const geometry::Ray& ray) const;

   LevelEditor& m_level_editor;

   bool m_is_being_used = false;
   std::optional<Axis> m_transform_axis;
   geometry::BoundingBox m_scaler_x_bb{};
   geometry::BoundingBox m_scaler_y_bb{};
   geometry::BoundingBox m_scaler_z_bb{};
   geometry::BoundingBox m_scaler_xyz_bb{};

   Transform3D m_starting_transform{};
   Vector3 m_starting_position{};
   Vector3 m_starting_point{};
   Vector3 m_starting_closest{};
};

}// namespace triglav::editor
