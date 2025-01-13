#pragma once

#include "IStage.hpp"

#include "triglav/graphics_api/Buffer.hpp"

namespace triglav::renderer::stage {

class AmbientOcclusionStage final : public IStage
{
 public:
   explicit AmbientOcclusionStage(graphics_api::Device& device);

   void build_stage(render_core::BuildContext& ctx) const override;

   void fill_sample_buffer();

 private:
   graphics_api::Buffer m_sampleBuffer;
};

}// namespace triglav::renderer::stage
