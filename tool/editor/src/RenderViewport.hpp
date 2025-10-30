#pragma once

#include "LevelEditor.hpp"

#include "triglav/render_core/JobGraph.hpp"

#include <vector>

namespace triglav::editor {

class RenderViewport
{
 public:
   explicit RenderViewport(LevelEditor& levelEditor, Vector4 dimensions);
   void build_update_job(render_core::BuildContext& ctx);
   void build_render_job(render_core::BuildContext& ctx);
   void update(render_core::JobGraph& graph, u32 frameIndex, float deltaTime);

   [[nodiscard]] Vector4 dimensions() const;

   void set_selection_matrix(u32 index, const Matrix4x4& mat);

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
   std::array<Vector4, 4> m_colors{
      Vector4(1, 0.5, 0, 1),
      Vector4(1, 0.09, 0.09, 1),
      Vector4(0.09, 1, 0.25, 1),
      Vector4(0.01, 0.25, 1, 1),
   };
};

}// namespace triglav::editor
