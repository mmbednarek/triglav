#include "Particles.h"

#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/RenderGraph.h"

#include <glm/vec3.hpp>

#include <random>

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::BufferUsage;
using graphics_api::PipelineStage;
using graphics_api::DescriptorType;

struct PushConstants {
   float deltaTime;
   u32 randomValue;
};

constexpr u32 g_particleCount = 256;

struct Particle
{
   alignas(16) glm::vec3 position;
   alignas(16) glm::vec3 velocity;
   float animation;
   float rotation;
   float angularVelocity;
   float scale;
};

static graphics_api::Buffer generate_particles(graphics_api::Device &device, const glm::vec3 &center,
                                               const glm::vec3 &range)
{
   std::vector<Particle> particles;
   particles.resize(g_particleCount);

   std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
   std::default_random_engine generator{};

   for (auto &particle : particles) {
      particle.position = center + range * glm::vec3(dist(generator), dist(generator), dist(generator));
      particle.velocity = glm::vec3(dist(generator), dist(generator), 0.0f) * 0.01f;
      particle.animation = 0.5f * (1.0f + dist(generator));
      particle.rotation = 2 * M_PI * dist(generator);
      particle.angularVelocity = dist(generator);
      particle.scale = 0.5f + (1.0f + dist(generator));
   }

   auto buffer = GAPI_CHECK(device.create_buffer(BufferUsage::StorageBuffer | BufferUsage::TransferDst,
                                                 sizeof(Particle) * particles.size()));
   GAPI_CHECK_STATUS(buffer.write_indirect(particles.data(), sizeof(Particle) * particles.size()));

   return buffer;
}

ParticlesResources::ParticlesResources(graphics_api::Device &device) :
    m_device(device),
    m_particlesBuffer(generate_particles(m_device, {-30, 0, -30}, {2, 2, 2}))
{
}

graphics_api::Buffer &ParticlesResources::particles_buffer()
{
   return m_particlesBuffer;
}

Particles::Particles(graphics_api::Device &device, resource::ResourceManager &resourceManager,
                     render_core::RenderGraph &renderGraph) :
    m_device(device),
    m_renderGraph(renderGraph),
    m_computePipeline(GAPI_CHECK(graphics_api::ComputePipelineBuilder(device)
                                         .compute_shader(resourceManager.get("particles.cshader"_rc))
                                         .descriptor_binding(DescriptorType::StorageBuffer)
                                         .descriptor_binding(DescriptorType::StorageBuffer)
                                         .push_constant(PipelineStage::ComputeShader, sizeof(PushConstants))
                                         .use_push_descriptors(true)
                                         .build()))
{
}

graphics_api::WorkTypeFlags Particles::work_types() const
{
   return graphics_api::WorkType::Compute;
}

std::unique_ptr<render_core::NodeFrameResources> Particles::create_node_resources()
{
   return std::make_unique<ParticlesResources>(m_device);
}

void Particles::set_delta_time(float value)
{
   m_deltaTime = value;
}

void Particles::record_commands(render_core::FrameResources &frameResources,
                                render_core::NodeFrameResources &resources,
                                graphics_api::CommandList &cmdList)
{
   auto &currentResources  = dynamic_cast<ParticlesResources &>(resources);
   auto &previousResources = dynamic_cast<ParticlesResources &>(m_renderGraph.previous_frame_resources().node("particles"_name));

   cmdList.bind_pipeline(m_computePipeline);
   cmdList.bind_storage_buffer(0, previousResources.particles_buffer());
   cmdList.bind_storage_buffer(1, currentResources.particles_buffer());

   std::uniform_int_distribution<u32> dist(0, std::numeric_limits<u32>::max());

   PushConstants pushConstants{
           .deltaTime = m_deltaTime,
           .randomValue = dist(m_randomEngine),
   };
   cmdList.push_constant(PipelineStage::ComputeShader, pushConstants);

   cmdList.dispatch(g_particleCount, 1, 1);
}

}// namespace triglav::renderer::node