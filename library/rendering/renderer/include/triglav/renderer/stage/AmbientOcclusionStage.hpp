#pragma once

#include "IStage.hpp"

#include "triglav/graphics_api/Buffer.hpp"
#include "triglav/graphics_api/Texture.hpp"

namespace triglav::renderer::stage {

class AmbientOcclusionStage final : public IStage
{
 public:
   explicit AmbientOcclusionStage(graphics_api::Device& device);

   void build_stage(render_core::BuildContext& ctx, const Config& config) const override;

   void fill_sample_buffer();
   static std::vector<Vector2> generate_noise_texture(Vector2i dimensions);

 private:
   graphics_api::Device& m_device;
   graphics_api::Buffer m_sample_buffer;
   graphics_api::Texture m_noise_texture;
};

}// namespace triglav::renderer::stage
