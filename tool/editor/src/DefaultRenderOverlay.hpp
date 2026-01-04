#pragma once

#include "IRenderOverlay.hpp"

namespace triglav::editor {

class DefaultRenderOverlay final : public IRenderOverlay
{
 public:
   void build_update_job(render_core::BuildContext& ctx) override;
   void build_render_job(render_core::BuildContext& ctx) override;
   void update(render_core::JobGraph& graph, u32 frame_index, float delta_time) override;
   void set_dimensions(Vector4 dimensions);
   [[nodiscard]] Vector4 dimensions() const override;

 private:
   Vector4 m_dimensions{0, 0, 32, 32};
};

}// namespace triglav::editor