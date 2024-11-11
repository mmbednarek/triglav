#pragma once

#include "triglav/render_core/IRenderNode.hpp"

#include "Scene.hpp"
#include "ShadingRenderer.hpp"

namespace triglav::renderer::node {

struct ParticlesUBO
{
   glm::mat4 view;
   glm::mat4 proj;
};

class Shading : public render_core::IRenderNode
{
 public:
   Shading(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene);

   std::unique_ptr<render_core::NodeFrameResources> create_node_resources() override;
   [[nodiscard]] graphics_api::WorkTypeFlags work_types() const override;
   void record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                        graphics_api::CommandList& cmdList) override;

   [[nodiscard]] float gpu_time() const;

 private:
   graphics_api::Device& m_device;
   graphics_api::RenderTarget m_shadingRenderTarget;
   ShadingRenderer m_shadingRenderer;
   Scene& m_scene;
   graphics_api::Pipeline m_particlesPipeline;
   graphics_api::UniformBuffer<ParticlesUBO> m_particlesUBO;
   graphics_api::Texture& m_particlesTexture;
   graphics_api::TimestampArray m_timestampArray;
};

}// namespace triglav::renderer::node
