#include "AmbientOcclusionRenderer.h"

#include <cstring>
#include <random>

#include "triglav/graphics_api/CommandList.h"
#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/Framebuffer.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace triglav::renderer {

AmbientOcclusionRenderer::AmbientOcclusionRenderer(graphics_api::Device &device,
                                                   graphics_api::RenderTarget &renderTarget,
                                                   ResourceManager &resourceManager,
                                                   const graphics_api::Texture &noiseTexture) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(m_device, renderTarget)
                                   .fragment_shader(resourceManager.get<ResourceType::FragmentShader>(
                                           "ambient_occlusion.fshader"_name))
                                   .vertex_shader(resourceManager.get<ResourceType::VertexShader>(
                                           "ambient_occlusion.vshader"_name))
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::PipelineStage::FragmentShader)
                                   .enable_depth_test(false)
                                   .use_push_descriptors(true)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .build())),
    // m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 3, 1))),
    m_sampler(resourceManager.get<ResourceType::Sampler>("linear_repeat_mlod0.sampler"_name)),
    m_samplesSSAO(generate_sample_points(g_SampleCountSSAO)),
    m_uniformBuffer(m_device),
    m_noiseTexture(noiseTexture)
{
   std::memcpy(&m_uniformBuffer->samplesSSAO, m_samplesSSAO.data(),
               m_samplesSSAO.size() * sizeof(AlignedVec3));
}

void AmbientOcclusionRenderer::draw(render_core::FrameResources &resources,
                                    graphics_api::CommandList &cmdList,
                                    const glm::mat4 &cameraProjection) const
{
   m_uniformBuffer->cameraProjection = cameraProjection;

   cmdList.bind_pipeline(m_pipeline);

   /*
    graphics_api::DescriptorWriter<4> desc;
    desc.sampler(0, m_framebuffer.texture(1), m_sampler);
    desc.sampler(1, m_framebuffer.texture(2), m_sampler);
    desc.sampler(2, m_noiseTexture, m_sampler);
    desc.ubo(3, m_uniformBuffer);
    cmdList.push_descriptors(desc);
    */

   auto &gbuffer =
           resources.node<render_core::NodeFrameResources>("geometry"_name_id).framebuffer("gbuffer"_name_id);

   graphics_api::DescriptorWriter writer(m_device);
   writer.set_sampled_texture(0, gbuffer.texture(1), m_sampler);
   writer.set_sampled_texture(1, gbuffer.texture(2), m_sampler);
   writer.set_sampled_texture(2, m_noiseTexture, m_sampler);
   writer.set_uniform_buffer(3, m_uniformBuffer);
   cmdList.push_descriptors(0, writer);

   cmdList.draw_primitives(4, 0);
}

std::vector<AmbientOcclusionRenderer::AlignedVec3>
AmbientOcclusionRenderer::generate_sample_points(const size_t count)
{
   static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
   static std::default_random_engine generator{};

   int index{};
   std::vector<AlignedVec3> result{};
   result.resize(count);
   for (auto &outSample : result) {
      glm::vec3 sample{dist(generator) * 2.0 - 1.0, dist(generator) * 2.0 - 1.0, dist(generator)};
      sample = glm::normalize(sample);
      sample *= dist(generator);

      const float scale = static_cast<float>(index) / static_cast<float>(count);
      sample *= std::lerp(0.1f, 1.0f, scale * scale);
      outSample.value = sample;

      ++index;
   }

   return result;
}

}// namespace triglav::renderer