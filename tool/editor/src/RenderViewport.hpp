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

 private:
   LevelEditor& m_levelEditor;
   Vector4 m_dimensions{};
};

}// namespace triglav::editor
