#include "stage/AmbientOcclusionStage.hpp"

#include "Util.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include <Config.hpp>
#include <Renderer.hpp>
#include <random>

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;
namespace gapi = graphics_api;

constexpr auto g_sampleCount = 64;

AmbientOcclusionStage::AmbientOcclusionStage(gapi::Device& device) :
    m_sampleBuffer(GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::UniformBuffer | gapi::BufferUsage::TransferDst, sizeof(Vector3_Aligned16B) * g_sampleCount)))
{
   this->fill_sample_buffer();
}

void AmbientOcclusionStage::build_stage(render_core::BuildContext& ctx, const Config& config) const
{
   if (config.ambientOcclusion != AmbientOcclusionMethod::ScreenSpace) {
      if (config.ambientOcclusion != AmbientOcclusionMethod::RayTraced) {
         ctx.declare_screen_size_texture("ambient_occlusion.blurred"_name, GAPI_FORMAT(R, Float16));
      }
      return;
   }
   ctx.declare_render_target("ambient_occlusion.target"_name, GAPI_FORMAT(R, Float16));

   ctx.begin_render_pass("ambient_occlusion"_name, "ambient_occlusion.target"_name);

   ctx.bind_fragment_shader("ambient_occlusion.fshader"_rc);

   ctx.bind_samplable_texture(0, "gbuffer.position"_name);
   ctx.bind_samplable_texture(1, "gbuffer.normal"_name);
   ctx.bind_samplable_texture(2, "noise.tex"_rc);
   ctx.bind_uniform_buffer(3, "core.view_properties"_external);
   ctx.bind_uniform_buffer(4, &m_sampleBuffer);

   ctx.draw_full_screen_quad();

   ctx.end_render_pass();

   blur_texture(ctx, "ambient_occlusion.target"_name, "ambient_occlusion.blurred"_name, GAPI_FORMAT(R, Float16));
}

void AmbientOcclusionStage::fill_sample_buffer()
{
   static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
   static std::default_random_engine generator{};

   int index{};

   std::vector<Vector3_Aligned16B> result{};
   result.resize(g_sampleCount);

   for (auto& outSample : result) {
      glm::vec3 sample{dist(generator) * 2.0 - 1.0, dist(generator) * 2.0 - 1.0, dist(generator)};
      sample = glm::normalize(sample);
      sample *= dist(generator);

      const float scale = static_cast<float>(index) / static_cast<float>(g_sampleCount);
      sample *= std::lerp(0.1f, 1.0f, scale * scale);
      outSample.value = sample;

      ++index;
   }

   GAPI_CHECK_STATUS(m_sampleBuffer.write_indirect(result.data(), result.size() * sizeof(Vector3_Aligned16B)));
}

}// namespace triglav::renderer::stage
