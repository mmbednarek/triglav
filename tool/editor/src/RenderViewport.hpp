#pragma once

#include "LevelEditor.hpp"

#include "triglav/render_core/JobGraph.hpp"

#include <vector>

namespace triglav::editor {

constexpr auto ARROW_HEIGHT = 4.3f;
constexpr auto TIP_HEIGHT = 1.3f;
constexpr auto SHAFT_HEIGHT = ARROW_HEIGHT - TIP_HEIGHT;
constexpr auto SHAFT_RADIUS = 0.08f;
constexpr auto TIP_RADIUS = 0.25f;

constexpr auto COLOR_X_AXIS = Color(1, 0.09, 0.09, 1);
constexpr auto COLOR_Y_AXIS = Color(0.04, 1, 0.20, 1);
constexpr auto COLOR_Z_AXIS = Color(0.01, 0.25, 1, 1);

constexpr auto COLOR_X_AXIS_HOVER = Color(1, 0.39, 0.39, 1);
constexpr auto COLOR_Y_AXIS_HOVER = Color(0.34, 1, 0.50, 1);
constexpr auto COLOR_Z_AXIS_HOVER = Color(0.31, 0.55, 1, 1);

class RenderViewport
{
 public:
   explicit RenderViewport(LevelEditor& levelEditor, Vector4 dimensions);
   void build_update_job(render_core::BuildContext& ctx);
   void build_render_job(render_core::BuildContext& ctx);
   void update(render_core::JobGraph& graph, u32 frameIndex, float deltaTime);

   [[nodiscard]] Vector4 dimensions() const;

   void set_selection_matrix(u32 index, const Matrix4x4& mat);
   void set_color(u32 index, const Color& color);

 private:
   LevelEditor& m_levelEditor;
   Vector4 m_dimensions{};
   u32 m_updates = 0;
   std::array<Matrix4x4, 4> m_matrices{
      Matrix4x4{0},
      Matrix4x4{0},
      Matrix4x4{0},
      Matrix4x4{0},
   };
   std::array<Color, 4> m_colors{
      Color(1, 0.5, 0, 1),
      Color(1, 0.09, 0.09, 1),
      Color(0.09, 1, 0.25, 1),
      Color(0.01, 0.25, 1, 1),
   };
};

}// namespace triglav::editor
