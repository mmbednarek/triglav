#pragma once

#include "ILevelEditorTool.hpp"

#include "triglav/Logging.hpp"

namespace triglav::editor {

class LevelEditor;

class ScalingTool final : public ILevelEditorTool
{
   TG_DEFINE_LOG_CATEGORY(ScalingTool)
 public:
   explicit ScalingTool(LevelEditor& levelEditor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;

 private:
   LevelEditor& m_levelEditor;

   std::optional<Axis> m_transformAxis;
   geometry::BoundingBox m_scaler_x_bb{};
   geometry::BoundingBox m_scaler_y_bb{};
   geometry::BoundingBox m_scaler_z_bb{};

   Vector3 m_startingTranslation{};
   Vector3 m_startingPosition{};
   Vector3 m_startingClosest{};
   Vector3 m_baseScale{};
};

}// namespace triglav::editor
