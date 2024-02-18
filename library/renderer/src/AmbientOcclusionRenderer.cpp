#include "AmbientOcclusionRenderer.h"

#include <cstring>
#include <random>

#include "graphics_api/CommandList.h"
#include "graphics_api/DescriptorWriter.h"
#include "graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderCore.hpp"

using namespace triglav::name_literals;
using triglav::ResourceType;
using triglav::render_core::checkResult;
using triglav::resource::ResourceManager;

namespace renderer {

AmbientOcclusionRenderer::AmbientOcclusionRenderer(graphics_api::Device &device,
                                                   graphics_api::RenderPass &renderPass,
                                                   ResourceManager &resourceManager,
                                                   const graphics_api::Texture &positionTexture,
                                                   const graphics_api::Texture &normalTexture,
                                                   const graphics_api::Texture &noiseTexture) :
    m_device(device),
    m_pipeline(checkResult(graphics_api::PipelineBuilder(m_device, renderPass)
                                   .fragment_shader(resourceManager.get<ResourceType::FragmentShader>(
                                           "ambient_occlusion.fshader"_name))
                                   .vertex_shader(resourceManager.get<ResourceType::VertexShader>(
                                           "ambient_occlusion.vshader"_name))
                                   // Vertex description
                                   .begin_vertex_layout<geometry::Vertex>()
                                   .end_vertex_layout()
                                   // Descriptor layout
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                                       graphics_api::ShaderStage::Fragment)
                                   .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                       graphics_api::ShaderStage::Fragment)
                                   .enable_depth_test(false)
                                   .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                                   .build())),
    m_descriptorPool(checkResult(m_pipeline.create_descriptor_pool(1, 3, 1))),
    m_sampler(checkResult(device.create_sampler(false))),
    m_descriptors(checkResult(m_descriptorPool.allocate_array(1))),
    m_samplesSSAO(generate_sample_points(g_SampleCountSSAO)),
    m_uniformBuffer(m_device)
{
   std::memcpy(&m_uniformBuffer->samplesSSAO, m_samplesSSAO.data(),
               m_samplesSSAO.size() * sizeof(AlignedVec3));

   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, positionTexture, m_sampler);
   writer.set_sampled_texture(1, normalTexture, m_sampler);
   writer.set_sampled_texture(2, noiseTexture, m_sampler);
   writer.set_uniform_buffer(3, m_uniformBuffer);
}

void AmbientOcclusionRenderer::update_textures(const graphics_api::Texture &positionTexture,
                                               const graphics_api::Texture &normalTexture,
                                               const graphics_api::Texture &noiseTexture) const
{
   graphics_api::DescriptorWriter writer(m_device, m_descriptors[0]);
   writer.set_sampled_texture(0, positionTexture, m_sampler);
   writer.set_sampled_texture(1, normalTexture, m_sampler);
   writer.set_sampled_texture(2, noiseTexture, m_sampler);
   writer.set_uniform_buffer(3, m_uniformBuffer);
}

void AmbientOcclusionRenderer::draw(graphics_api::CommandList &cmdList,
                                    const glm::mat4 &cameraProjection) const
{
   // m_samplesSSAO = generate_sample_points(g_SampleCountSSAO);
   // std::memcpy(&m_uniformBuffer->samplesSSAO, m_samplesSSAO.data(),
   //             m_samplesSSAO.size() * sizeof(AlignedVec3));

   m_uniformBuffer->cameraProjection = cameraProjection;

   cmdList.bind_pipeline(m_pipeline);
   cmdList.bind_descriptor_set(m_descriptors[0]);

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

}// namespace renderer