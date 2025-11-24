#pragma once

#include "LevelEditor.hpp"

#include "triglav/render_core/JobGraph.hpp"

#include <vector>

namespace triglav::editor {

constexpr auto ARROW_HEIGHT = 5.0f;
constexpr auto TIP_HEIGHT = 1.3f;
constexpr auto SHAFT_HEIGHT = ARROW_HEIGHT - TIP_HEIGHT;
constexpr auto SHAFT_RADIUS = 0.08f;
constexpr auto TIP_RADIUS = 0.25f;
constexpr auto BOX_SIZE = 0.25f;
constexpr auto SCALING_CUBE = 0.25f;
constexpr auto ROTATOR_RADIUS = 4.0f;
constexpr auto ROTATOR_WIDTH = 0.5f;

constexpr auto COLOR_X_AXIS = Color(1, 0.09, 0.09, 1);
constexpr auto COLOR_Y_AXIS = Color(0.04, 1, 0.20, 1);
constexpr auto COLOR_Z_AXIS = Color(0.01, 0.25, 1, 1);
constexpr auto COLOR_XYZ_AXIS = Color(0.7, 0.7, 0.7, 1);

constexpr auto COLOR_X_AXIS_HOVER = Color(1, 0.39, 0.39, 1);
constexpr auto COLOR_Y_AXIS_HOVER = Color(0.34, 1, 0.50, 1);
constexpr auto COLOR_Z_AXIS_HOVER = Color(0.31, 0.55, 1, 1);
constexpr auto COLOR_XYZ_AXIS_HOVER = Color(1, 1, 1, 1);

constexpr auto OVERLAY_SELECTION_BOX = 0;
constexpr auto OVERLAY_ARROW_X = 1;
constexpr auto OVERLAY_ARROW_Y = 2;
constexpr auto OVERLAY_ARROW_Z = 3;

constexpr auto OVERLAY_ROTATOR_X = 4;
constexpr auto OVERLAY_ROTATOR_Y = 5;
constexpr auto OVERLAY_ROTATOR_Z = 6;

constexpr auto OVERLAY_SCALER_X = 7;
constexpr auto OVERLAY_SCALER_Y = 8;
constexpr auto OVERLAY_SCALER_Z = 9;

constexpr auto OVERLAY_SCALER_XYZ = 10;

constexpr auto OVERLAY_COUNT = 11;


class RenderViewport
{
 public:
   explicit RenderViewport(LevelEditor& level_editor, Vector4 dimensions);
   void build_update_job(render_core::BuildContext& ctx);
   void build_render_job(render_core::BuildContext& ctx);
   void update(render_core::JobGraph& graph, u32 frame_index, float delta_time);
   void reset_matrices();

   [[nodiscard]] Vector4 dimensions() const;

   void set_selection_matrix(u32 index, const Matrix4x4& mat);
   void set_color(u32 index, const Color& color);

 private:
   LevelEditor& m_level_editor;
   Vector4 m_dimensions{};
   u32 m_updates = 0;
   std::array<Matrix4x4, OVERLAY_COUNT> m_matrices{
      Matrix4x4{0},
   };
   std::array<Color, OVERLAY_COUNT> m_colors{
      Color(1, 0.5, 0, 1), COLOR_X_AXIS, COLOR_Y_AXIS, COLOR_Z_AXIS, COLOR_X_AXIS,   COLOR_Y_AXIS,
      COLOR_Z_AXIS,        COLOR_X_AXIS, COLOR_Y_AXIS, COLOR_Z_AXIS, COLOR_XYZ_AXIS,
   };
};

}// namespace triglav::editor
