#include "stage/AmbientOcclusionStage.hpp"

#include "Config.hpp"
#include "Renderer.hpp"
#include "Util.hpp"

#include "triglav/render_core/BuildContext.hpp"

#include <random>

namespace triglav::renderer::stage {

using namespace name_literals;
using namespace render_core::literals;
namespace gapi = graphics_api;

constexpr auto g_sample_count = 64;

AmbientOcclusionStage::AmbientOcclusionStage(gapi::Device& device) :
    m_device(device),
    m_sample_buffer(GAPI_CHECK(device.create_buffer(gapi::BufferUsage::UniformBuffer | gapi::BufferUsage::TransferDst,
                                                    sizeof(Vector3_Aligned16B) * g_sample_count))),
    m_noise_texture(GAPI_CHECK(m_device.create_texture(GAPI_FORMAT(RG, UNorm8), {64, 64})))
{
   this->fill_sample_buffer();

   auto noise_vec = generate_noise_texture({64, 64});
   auto noise_stage =
      GAPI_CHECK(m_device.create_buffer(gapi::BufferUsage::TransferSrc | gapi::BufferUsage::HostVisible, 2 * sizeof(char) * 64 * 64));
   {
      const auto mapped = GAPI_CHECK(noise_stage.map_memory());
      auto* data = static_cast<std::pair<u8, u8>*>(*mapped);
      std::ranges::transform(noise_vec, data, [](const Vector2 vec) {
         const auto val = 255.0f * (vec + 1.0f);
         return std::make_pair(static_cast<u8>(val.x), static_cast<u8>(val.y));
      });
   }

   auto cmd_list = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));
   GAPI_CHECK_STATUS(cmd_list.begin(graphics_api::SubmitType::OneTime));

   cmd_list.texture_barrier(graphics_api::PipelineStage::Entrypoint, graphics_api::PipelineStage::Transfer,
                            graphics_api::TextureBarrierInfo{
                               .texture = &m_noise_texture,
                               .source_state = graphics_api::TextureState::Undefined,
                               .target_state = graphics_api::TextureState::TransferDst,
                               .base_mip_level = 0,
                               .mip_level_count = 1,
                            });

   cmd_list.copy_buffer_to_texture(noise_stage, m_noise_texture);

   cmd_list.texture_barrier(graphics_api::PipelineStage::Transfer, graphics_api::PipelineStage::FragmentShader,
                            graphics_api::TextureBarrierInfo{
                               .texture = &m_noise_texture,
                               .source_state = graphics_api::TextureState::TransferDst,
                               .target_state = graphics_api::TextureState::ShaderRead,
                               .base_mip_level = 0,
                               .mip_level_count = 1,
                            });

   GAPI_CHECK_STATUS(cmd_list.finish());

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(cmd_list));
}

void AmbientOcclusionStage::build_stage(render_core::BuildContext& ctx, const Config& config) const
{
   if (config.ambient_occlusion != AmbientOcclusionMethod::ScreenSpace) {
      if (config.ambient_occlusion != AmbientOcclusionMethod::RayTraced) {
         ctx.declare_screen_size_texture("ambient_occlusion.blurred"_name, GAPI_FORMAT(R, Float16));
      }
      return;
   }
   ctx.declare_render_target("ambient_occlusion.target"_name, GAPI_FORMAT(R, Float16));

   // ctx.query_timestamp(0, false);

   ctx.begin_render_pass("ambient_occlusion"_name, "ambient_occlusion.target"_name);

   ctx.bind_fragment_shader("ambient_occlusion.fshader"_rc);

   ctx.bind_texture(0, "gbuffer.position"_name);
   ctx.bind_texture(1, "gbuffer.normal"_name);
   ctx.bind_texture(2, &m_noise_texture);
   ctx.bind_uniform_buffer(3, "core.view_properties"_external);
   ctx.bind_uniform_buffer(4, &m_sample_buffer);

   ctx.draw_full_screen_quad();

   ctx.end_render_pass();

   blur_texture(ctx, "ambient_occlusion.target"_name, "ambient_occlusion.blurred"_name, GAPI_FORMAT(R, Float16));

   // ctx.query_timestamp(1, true);
}

void AmbientOcclusionStage::fill_sample_buffer()
{
   static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
   static std::default_random_engine generator{};

   MemorySize index{};

   std::vector<Vector3_Aligned16B> result{};
   result.resize(g_sample_count);

   static constexpr auto min_distance = 0.25f;

   for (auto& out_sample : result) {
      Vector3 sample;
      for (;;) {
         sample = {dist(generator) * 2.0 - 1.0, dist(generator) * 2.0 - 1.0, dist(generator)};
         sample = glm::normalize(sample);
         sample *= dist(generator);

         const float scale = static_cast<float>(index) / static_cast<float>(g_sample_count);
         sample *= std::lerp(0.1f, 1.0f, scale * scale);

         const auto length = glm::length(sample);

         bool found_close = false;
         for (MemorySize i = 0; i < index; ++i) {
            const auto sample_dist = glm::distance(sample, result[i].value);
            if (sample_dist < min_distance * length) {
               found_close = true;
               break;
            }
         }

         if (!found_close)
            break;
      }

      out_sample.value = sample;
      ++index;
   }

   GAPI_CHECK_STATUS(m_sample_buffer.write_indirect(result.data(), result.size() * sizeof(Vector3_Aligned16B)));
}

std::vector<Vector2> AmbientOcclusionStage::generate_noise_texture(const Vector2i dimensions)
{
   std::vector<Vector2> vectors(dimensions.x * dimensions.y);

   static std::default_random_engine generator{2957831};
   static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

   static constexpr auto min_distance = 0.0005f;

   [[maybe_unused]] u32 skipped = 0;

   MemorySize index = 0;
   for (int y = 0; y < dimensions.y; ++y) {
      for (int x = 0; x < dimensions.x; ++x) {
         Vector2 vec;
         for (;;) {
            vec = {dist(generator), dist(generator)};
            vec = glm::normalize(vec);

            bool found_close = false;
            for (MemorySize i = 0; i < index; ++i) {
               if (glm::distance(vec, vectors[i]) < min_distance) {
                  found_close = true;
                  ++skipped;
                  break;
               }
            }
            if (!found_close) {
               break;
            }
         }

         vectors[index] = vec;
         ++index;
      }
   }

   return vectors;
}

}// namespace triglav::renderer::stage
